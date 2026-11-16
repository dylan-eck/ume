#include "renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/core/resource_handle.hpp"
#include "ume/platform/window.hpp"
#include "ume/renderer/backend_types.hpp"
#include "ume/renderer/renderer_backend.hpp"
#include "ume/renderer/camera.hpp"
#include "ume/renderer/shader_compiler.hpp"
#include "ume/core/error.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include <span>

namespace ume {

namespace {
const EntryPoint *findEntryPoint(std::span<const EntryPoint> entry_points,
                                 ShaderStage stage) {
    auto itr = std::ranges::find(entry_points, stage, &EntryPoint::stage);
    return itr != entry_points.end() ? &*itr : nullptr;
}

const ShaderBinding *findBinding(std::span<const ShaderBinding> bindings,
                                 std::string_view name) {
    auto itr = std::ranges::find(bindings, name, &ShaderBinding::name);
    return itr != bindings.end() ? &*itr : nullptr;
}

} // namespace

struct DrawUniforms {
    glm::mat4 model_view_projection;
    glm::mat4 model;  // mesh local -> camera-relative world
    glm::mat4 normal; // normal matrix
    glm::vec4 base_color = glm::vec4(1.0f);
    // world origin in camera-relative space (i.e. -camera_position), so shaders
    // can recover an absolute world position in float precision
    glm::vec4 world_origin;
};

struct PostFrameUniforms {
    glm::mat4 inverse_projection;
    glm::mat4 camera_to_world;
    glm::vec2 resolution;
    float z_near;
    float fov_y;
    float aspect;
    std::array<float, 3> pad{};
};
static_assert(sizeof(PostFrameUniforms) == 160);

Renderer::Renderer(const Window &window)
    : pixel_width_(window.getPixelWidth()),
      pixel_height_(window.getPixelHeight()),
      aspect_(static_cast<float>(pixel_width_) /
              static_cast<float>(pixel_height_)),
      backend_(createRendererBackend(window)),
      compiler_(std::make_unique<ShaderCompiler>(
          backend_->shaderTarget(),
          std::filesystem::path(UME_SOURCE_DIR) / "include")) {

    if (!createRenderTargets()) {
        throw Error(logger::Category::Renderer,
                    "failed to create render targets");
    }

    linear_sampler_ = backend_->createSampler({
        .min_filter = Filter::Linear,
        .mag_filter = Filter::Linear,
        .address_mode = AddressMode::ClampToEdge,
    });

    if (!linear_sampler_) {
        throw Error(logger::Category::Renderer,
                    "failed to create linear sampler");
    }

    std::optional<CompiledShader> default_comp =
        compiler_->compile(std::filesystem::path(UME_SOURCE_DIR) /
                           "src/ume/renderer/shaders/default.slang");

    if (default_comp == std::nullopt) {
        throw Error(logger::Category::Renderer,
                    "failed to compile default shader");
    }

    const EntryPoint *vertex_entry =
        findEntryPoint(default_comp->entry_points, ShaderStage::Vertex);
    const EntryPoint *fragment_entry =
        findEntryPoint(default_comp->entry_points, ShaderStage::Fragment);

    if (vertex_entry == nullptr) {
        throw Error(logger::Category::Renderer,
                    "failed to find vertex entry point in default shader");
    }

    if (fragment_entry == nullptr) {
        throw Error(logger::Category::Renderer,
                    "failed to find fragment entry point in default shader");
    }

    const ShaderBinding *draw_uniforms =
        findBinding(default_comp->bindings, "draw");
    const ShaderBinding *vertex_buffer =
        findBinding(default_comp->bindings, "vertices");

    if (draw_uniforms == nullptr ||
        draw_uniforms->type != BindingType::Buffer ||
        draw_uniforms->size != sizeof(DrawUniforms)) {
        throw Error(
            logger::Category::Renderer,
            "draw uniforms binding in default shader is missing or invalid");
    }

    draw_uniforms_slot_ = draw_uniforms->slot;

    if (vertex_buffer == nullptr ||
        vertex_buffer->type != BindingType::Buffer) {
        throw Error(
            logger::Category::Renderer,
            "vertex buffer binding in default shader is missing or invalid");
    }

    vertex_buffer_slot_ = vertex_buffer->slot;

    default_shader_ = backend_->createShader({
        .code = default_comp->code,
        .entry_points = default_comp->entry_points,
    });

    default_pipeline_ = backend_->createGraphicsPipeline({
        .shader = default_shader_,
        .vertex_entry = vertex_entry->name,
        .fragment_entry = fragment_entry->name,
        .color_formats = {Format::BGRA8Unorm},
        .depth_format = Format::Depth32Float,
        .depth_state =
            {
                .compare = CompareFunction::Greater,
                .write = true,
            },
        .raster_state =
            {
                .cull_mode = CullMode::Back,
                .front_face_winding = Winding::CounterClockwise,
            },
    });

    if (!default_pipeline_) {
        throw Error(logger::Category::Renderer,
                    "failed to build default pipeline");
    }
}

Renderer::~Renderer() = default;

void Renderer::resize(uint32_t width, uint32_t height) {
    if (width == pixel_width_ && height == pixel_height_) return;

    UME_LOG_INFO(Renderer, "window resized: {}, {}", width, height);

    pixel_width_ = width;
    pixel_height_ = height;

    if (height != 0) {
        aspect_ = static_cast<float>(width) / static_cast<float>(height);
    }

    backend_->resize(width, height);

    if (width > 0 && height > 0 && !createRenderTargets()) {
        UME_LOG_WARN(Renderer,
                     "failed to recreate render targets during resize");
    }
}

MeshHandle Renderer::createMesh(const MeshDescription &desc) {
    // TODO: input validation / error handling

    const size_t vertex_count = desc.positions.size() / 3;

    std::vector<Vertex> vertices;
    vertices.reserve(vertex_count);

    for (size_t i = 0; i < vertex_count; i++) {
        const size_t base_idx = i * 3;

        vertices.push_back(Vertex{
            .position = glm::vec4(desc.positions[base_idx],
                                  desc.positions[base_idx + 1],
                                  desc.positions[base_idx + 2], 1.0f),
            .normal =
                glm::vec4(desc.normals[base_idx], desc.normals[base_idx + 1],
                          desc.normals[base_idx + 2], 0.0f),
        });
    }

    BufferHandle vertex_buffer = backend_->createBuffer({
        .size = vertices.size() * sizeof(vertices[0]),
        .initial_data = vertices.data(),
        .usage = BufferUsage::CpuToGpu,
    });
    if (!vertex_buffer) return {};

    BufferHandle index_buffer = backend_->createBuffer({
        .size = desc.indices.size_bytes(),
        .initial_data = desc.indices.data(),
        .usage = BufferUsage::CpuToGpu,
    });
    if (!index_buffer) {
        backend_->destroyBuffer(vertex_buffer);
        return {};
    }

    MeshHandle handle = meshes_.insert({
        .vertex_buffer = vertex_buffer,
        .index_buffer = index_buffer,
        .index_count = static_cast<uint32_t>(desc.indices.size()),
    });

    return handle;
}

void Renderer::destroyMesh(MeshHandle handle) {
    std::optional<Mesh> mesh = meshes_.remove(handle);
    if (!mesh) {
        UME_LOG_WARN(Renderer, "attempted to destroy stale mesh handle: {}",
                     handle.id);
        return;
    }

    backend_->destroyBuffer(mesh->vertex_buffer);
    backend_->destroyBuffer(mesh->index_buffer);
}

void Renderer::submit(MeshHandle handle, const glm::dvec3 &world_position,
                      const glm::mat4 &local_transform) {
    Mesh *mesh = meshes_.get(handle);
    if (mesh == nullptr) {
        UME_LOG_WARN(Renderer, "invalid mesh handle submitted {}", handle.id);
        return;
    }

    submissions_.push_back({
        .mesh = *mesh,
        .world_position = world_position,
        .local_transform = local_transform,
        .base_color = debugColorFromId(handle.id),
    });
}

PostEffectHandle
Renderer::createPostEffect(const std::filesystem::path &shader_path) {
    std::optional<CompiledShader> compiled = compiler_->compile(shader_path);
    if (!compiled) return {};

    ShaderHandle shader = backend_->createShader({
        .code = compiled->code,
        .entry_points = compiled->entry_points,
    });
    if (!shader) return {};

    auto bindings = resolvePostEffectBindings(*compiled, shader_path);
    if (!bindings) {
        UME_LOG_ERROR(Renderer, "failed to resolve bindings for post effect {}",
                      shader_path.string());
        backend_->destroyShader(shader);
        return {};
    }

    GraphicsPipelineHandle pipeline =
        createPostEffectPipeline(shader, *compiled, shader_path);
    if (!pipeline) {
        backend_->destroyShader(shader);
        return {};
    }

    return post_effects_.insert({
        .shader = shader,
        .pipeline = pipeline,
        .source_path = shader_path,
        .bindings = *bindings,
    });
}

bool Renderer::reloadPostEffect(PostEffectHandle handle) {
    PostEffect *effect = post_effects_.get(handle);
    if (effect == nullptr) return false;
    std::optional<CompiledShader> compiled =
        compiler_->compile(effect->source_path);
    if (!compiled) {
        return false; // keep the last good pipeline; the error is already
                      // logged
    }

    // build the replacements before tearing anything down, so a bad recompile
    // leaves the last good shader and pipeline in place
    ShaderHandle shader = backend_->createShader({
        .code = compiled->code,
        .entry_points = compiled->entry_points,
    });
    if (!shader) return false;

    auto bindings = resolvePostEffectBindings(*compiled, effect->source_path);
    if (!bindings) {
        UME_LOG_ERROR(Renderer, "failed to resolve bindings for post effect {}",
                      effect->source_path.string());
        backend_->destroyShader(shader);
        return false;
    }

    GraphicsPipelineHandle pipeline =
        createPostEffectPipeline(shader, *compiled, effect->source_path);
    if (!pipeline) {
        backend_->destroyShader(shader);
        return false;
    }

    backend_->destroyGraphicsPipeline(effect->pipeline);
    backend_->destroyShader(effect->shader);

    effect->shader = shader;
    effect->pipeline = pipeline;
    effect->bindings = *bindings;
    UME_LOG_INFO(Renderer, "reloaded '{}'", effect->source_path.string());
    return true;
}

void Renderer::destroyPostEffect(PostEffectHandle handle) {
    std::optional<PostEffect> effect = post_effects_.remove(handle);
    if (!effect) {
        UME_LOG_WARN(Renderer,
                     "attempted to destroy stale post effect handle: {}",
                     handle.id);
        return;
    }
    backend_->destroyGraphicsPipeline(effect->pipeline);
    backend_->destroyShader(effect->shader);
}

void Renderer::submitPostEffect(PostEffectHandle handle,
                                std::span<const std::byte> params) {
    PostEffect *effect = post_effects_.get(handle);
    if (effect == nullptr) {
        UME_LOG_WARN(Renderer, "invalid post effect handle submitted {}",
                     handle.id);
        return;
    }
    if (params.size() != effect->bindings.params_size) {
        UME_LOG_WARN(Renderer, "post effect {} expects {} param bytes, got {}",
                     handle.id, effect->bindings.params_size, params.size());
        return;
    }
    const size_t offset = post_param_arena_.size();
    post_param_arena_.insert(post_param_arena_.end(), params.begin(),
                             params.end());
    post_submissions_.push_back({
        .effect = handle,
        .param_offset = offset,
        .param_size = params.size(),
    });
}

void Renderer::render() {
    if (pixel_width_ == 0 || pixel_height_ == 0) return;

    backend_->beginFrame();
    backend_->beginRenderPass({
        .color_attachments =
            {
                ColorAttachment{
                    .texture = scene_color_,
                    .clear_value = {0, 0, 0, 1},
                },
            },
        .depth_attachment = DepthAttachment{.texture = depth_},
    });

    const glm::mat4 projection =
        perspectiveReverseZ(camera_state_.fov_y, aspect_, camera_state_.z_near);

    const auto view_rotation =
        glm::mat4(glm::transpose(camera_state_.orientation));

    for (const auto &next : submissions_) {
        const auto relative =
            glm::vec3(next.world_position - camera_state_.position);

        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), relative) * next.local_transform;

        Mesh mesh = next.mesh;
        DrawUniforms uniforms{
            .model_view_projection = projection * view_rotation * model,
            .model = model,
            .normal = glm::transpose(glm::inverse(model)),
            .base_color = next.base_color,
            .world_origin = glm::vec4(glm::vec3(-camera_state_.position), 1.0f),
        };

        const std::array<BufferBinding, 1> vertex_buffers = {
            BufferBinding{
                .slot = vertex_buffer_slot_,
                .buffer = mesh.vertex_buffer,
                .offset = 0,
            },
        };

        const std::array<InlineDataBinding, 1> inline_data = {
            InlineDataBinding{
                .slot = draw_uniforms_slot_,
                .data = std::as_bytes(std::span(&uniforms, 1)),
            },
        };

        backend_->draw({
            .pipeline = default_pipeline_,
            .index_count = mesh.index_count,
            .index_buffer = mesh.index_buffer,
            .index_type = mesh.index_type,
            .vertex_bindings{
                .buffers = vertex_buffers,
                .inline_data = inline_data,
            },

        });
    }
    submissions_.clear();

    backend_->endRenderPass();

    const PostFrameUniforms frame_uniforms{
        .inverse_projection = glm::inverse(projection),
        .camera_to_world = glm::mat4(camera_state_.orientation),
        .resolution = glm::vec2(pixel_width_, pixel_height_),
        .z_near = camera_state_.z_near,
        .fov_y = camera_state_.fov_y,
        .aspect = aspect_,
    };

    struct PostPass {
        const PostEffect *effect;
        std::span<const std::byte> params;
    };

    std::vector<PostPass> passes;
    passes.reserve(post_submissions_.size());
    for (const PostSubmission &s : post_submissions_) {
        const PostEffect *effect = post_effects_.get(s.effect);
        if (effect == nullptr) continue; // destroyed after it was submitted
        passes.push_back({
            .effect = effect,
            .params = std::span<const std::byte>(post_param_arena_)
                          .subspan(s.param_offset, s.param_size),
        });
    }

    if (passes.empty()) {
        backend_->copyTexture(scene_color_, backend_->backbuffer());
    } else {
        TextureHandle src = scene_color_;

        for (size_t i = 0; i < passes.size(); i++) {
            const PostEffect *effect = passes[i].effect;

            TextureHandle dst;
            if (i == passes.size() - 1) {
                dst = backend_->backbuffer();
            } else if (src == scene_color_) {
                dst = post_color_;
            } else {
                dst = scene_color_;
            }

            backend_->beginRenderPass({
                .color_attachments =
                    {
                        ColorAttachment{
                            .texture = dst,
                            .load_action = LoadAction::DontCare,
                        },
                    },
            });

            const PostEffectBindings &b = effect->bindings;

            std::array<TextureBinding, 2> textures{};
            size_t texture_count = 0;
            textures[texture_count++] = {
                .slot = b.scene_texture_slot,
                .texture = src,
            };

            if (b.depth_texture_slot) {
                textures[texture_count++] = {
                    .slot = *b.depth_texture_slot,
                    .texture = depth_,
                };
            }

            const std::array samplers{
                SamplerBinding{
                    .slot = b.scene_sampler_slot,
                    .sampler = linear_sampler_,
                },
            };

            std::array<InlineDataBinding, 2> inline_data{};
            size_t inline_data_count = 0;
            inline_data[inline_data_count++] = {
                .slot = b.frame_uniforms_slot,
                .data = std::as_bytes(std::span(&frame_uniforms, 1)),
            };

            if (b.params_slot && !passes[i].params.empty()) {
                inline_data[inline_data_count++] = {
                    .slot = *b.params_slot,
                    .data = passes[i].params,
                };
            }

            backend_->draw({
                .pipeline = effect->pipeline,
                .vertex_count = 3,
                .fragment_bindings =
                    {
                        .textures = std::span(textures).first(texture_count),
                        .samplers = samplers,
                        .inline_data =
                            std::span(inline_data).first(inline_data_count),
                    },
            });

            backend_->endRenderPass();
            src = dst;
        }
    }

    post_submissions_.clear();
    post_param_arena_.clear();

    backend_->endFrame();
}

void Renderer::setCamera(const CameraState &camera_state) {
    camera_state_ = camera_state;
}

bool Renderer::createRenderTargets() {
    backend_->destroyTexture(scene_color_);
    backend_->destroyTexture(post_color_);
    backend_->destroyTexture(depth_);

    scene_color_ = backend_->createTexture({
        .width = pixel_width_,
        .height = pixel_height_,
        .format = Format::BGRA8Unorm,
        .usage = TextureUsage::RenderTarget | TextureUsage::ShaderRead,
    });

    post_color_ = backend_->createTexture({
        .width = pixel_width_,
        .height = pixel_height_,
        .format = Format::BGRA8Unorm,
        .usage = TextureUsage::RenderTarget | TextureUsage::ShaderRead,
    });

    depth_ = backend_->createTexture({
        .width = pixel_width_,
        .height = pixel_height_,
        .format = Format::Depth32Float,
        .usage = TextureUsage::RenderTarget | TextureUsage::ShaderRead,
    });

    return scene_color_ && post_color_ && depth_;
}

std::optional<Renderer::PostEffectBindings>
Renderer::resolvePostEffectBindings(const CompiledShader &shader,
                                    const std::filesystem::path &path) {
    bool invalid_bindings = false;

    auto resolve_binding = [&](std::string_view name, BindingType type,
                               bool required = false) -> const ShaderBinding * {
        const ShaderBinding *binding = findBinding(shader.bindings, name);

        if (binding == nullptr) {
            if (required) {
                UME_LOG_ERROR(Renderer,
                              "post effect {} is missing required binding {}",
                              path.string(), name);
                invalid_bindings = true;
            }
            return nullptr;
        }

        if (binding->type != type) {
            UME_LOG_ERROR(Renderer,
                          "binding {} in post effect {} has incorrect type: "
                          "expected {}, got {}",
                          name, path.string(), toString(type),
                          toString(binding->type));
            invalid_bindings = true;
            return nullptr;
        }

        return binding;
    };

    const ShaderBinding *frame_uniforms =
        resolve_binding("frame", BindingType::Buffer, true);
    const ShaderBinding *scene_texture =
        resolve_binding("scene_texture", BindingType::Texture, true);
    const ShaderBinding *scene_sampler =
        resolve_binding("scene_sampler", BindingType::Sampler, true);
    const ShaderBinding *depth_texture =
        resolve_binding("depth_texture", BindingType::Texture, false);
    const ShaderBinding *params =
        resolve_binding("params", BindingType::Buffer, false);

    if (invalid_bindings) {
        return std::nullopt;
    }

    if (frame_uniforms->size != sizeof(PostFrameUniforms)) {
        UME_LOG_ERROR(Renderer,
                      "frame uniforms binding for post effect {} is incorrect "
                      "size: expected {}, got {}",
                      path.string(), sizeof(PostFrameUniforms),
                      frame_uniforms->size);
        return std::nullopt;
    }

    return PostEffectBindings{
        .frame_uniforms_slot = frame_uniforms->slot,
        .scene_texture_slot = scene_texture->slot,
        .scene_sampler_slot = scene_sampler->slot,
        .depth_texture_slot = depth_texture != nullptr
                                  ? std::optional(depth_texture->slot)
                                  : std::nullopt,
        .params_slot =
            params != nullptr ? std::optional(params->slot) : std::nullopt,
        .params_size = params != nullptr ? params->size : 0,
    };
}

GraphicsPipelineHandle
Renderer::createPostEffectPipeline(ShaderHandle shader,
                                   const CompiledShader &compiled,
                                   const std::filesystem::path &path) {

    const EntryPoint *vertex_entry =
        findEntryPoint(compiled.entry_points, ShaderStage::Vertex);
    const EntryPoint *fragment_entry =
        findEntryPoint(compiled.entry_points, ShaderStage::Fragment);

    if (vertex_entry == nullptr) {
        UME_LOG_ERROR(Renderer, "vertex entry not found in post effect {}",
                      path.string());
        return {};
    }

    if (fragment_entry == nullptr) {
        UME_LOG_ERROR(Renderer, "fragment entry not found in post effect {}",
                      path.string());
        return {};
    }

    GraphicsPipelineHandle pipeline = backend_->createGraphicsPipeline({
        .shader = shader,
        .vertex_entry = vertex_entry->name,
        .fragment_entry = fragment_entry->name,
        .color_formats = {Format::BGRA8Unorm},
    });
    if (!pipeline) {
        return {};
    }

    return pipeline;
}

} // namespace ume