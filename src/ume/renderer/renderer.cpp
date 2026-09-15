#include "renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/platform/window.hpp"
#include "ume/renderer/shader_compiler.hpp"
#include "ume/core/error.hpp"

#include <array>
#include <algorithm>

namespace ume {

namespace {
uint32_t postEffectParamsSize(const CompiledShader &shader) {
    auto itr =
        std::ranges::find(shader.bindings, "params", &ShaderBinding::name);
    return itr == shader.bindings.end() ? 0 : itr->size;
}
} // namespace

// TODO: better naming?
struct DrawUniforms {
    glm::mat4 model_view_projection;
    glm::mat4 normal; // normal matrix
    glm::vec4 base_color = glm::vec4(1.0f);
};

// TODO: reorder this?
struct PostFrameUniforms {
    glm::mat4 inverse_projection;
    glm::mat4 camera_to_world;
    glm::vec2 resolution;
    float z_near;
    float fov_y;
    float aspect;
    std::array<float, 3> _pad;
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

    std::optional<CompiledShader> default_comp =
        compiler_->compile(std::filesystem::path(UME_SOURCE_DIR) /
                           "src/ume/renderer/shaders/default.slang");

    if (default_comp != std::nullopt) {
        default_shader_ = backend_->createShader({
            .code = default_comp->code,
            .entry_points = default_comp->entry_points,
        });
    }

    default_pipeline_ = backend_->createGraphicsPipeline({
        .shader = default_shader_,
        .vertex_entry = "vertMain",
        .fragment_entry = "fragMain",
        .color_format = Format::BGRA8Unorm,
        .depth_format = Format::Depth32Float,
    });

    if (!default_pipeline_) {
        throw Error(logger::Category::Renderer,
                    "failed to build default pipeline");
    }

    const auto path = std::filesystem::path(UME_SOURCE_DIR) /
                      "src/ume/renderer/shaders/compute_test.slang";
    std::optional<CompiledShader> comp = compiler_->compile(path);

    if (comp == std::nullopt) {
        UME_LOG_WARN(Renderer, "failed to compile compute test shader");
        compute_test_shader_ = {};
    } else {
        compute_test_shader_ = backend_->createShader({
            .code = comp->code,
            .entry_points = comp->entry_points,
        });

        compute_test_pipeline_ = backend_->createComputePipeline({
            .shader = compute_test_shader_,
            .entry = "doubleArray",
        });

        std::array<float, 1000> zeroes{};

        std::array<float, 1000> ramp{};
        for (size_t i = 0; i < ramp.size(); i++) {
            ramp[i] = static_cast<float>(i);
        }

        input_ = backend_->createBuffer({.size = 1000 * sizeof(float),
                                         .initial_data = ramp.data(),
                                         .usage = BufferUsage::CpuToGpu});

        output_ = backend_->createBuffer({.size = 1000 * sizeof(float),
                                          .initial_data = zeroes.data(),
                                          .usage = BufferUsage::CpuToGpu});
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

    BufferHandle vertex_buffer =
        backend_->createBuffer({.size = vertices.size() * sizeof(vertices[0]),
                                .initial_data = vertices.data(),
                                .usage = BufferUsage::CpuToGpu});
    if (!vertex_buffer) return {};

    BufferHandle index_buffer =
        backend_->createBuffer({.size = desc.indices.size_bytes(),
                                .initial_data = desc.indices.data(),
                                .usage = BufferUsage::CpuToGpu});
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

    return {};
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

    submissions_.push_back({.mesh = *mesh,
                            .world_position = world_position,
                            .local_transform = local_transform,
                            .base_color = debugColorFromId(handle.id)});
}

PostEffectHandle
Renderer::createPostEffect(const std::filesystem::path &shader_path) {
    std::optional<CompiledShader> compiled = compiler_->compile(shader_path);
    if (!compiled) return {};

    ShaderHandle shader = backend_->createShader({
        .code = compiled->code,
        .entry_points = compiled->entry_points,
    });

    auto itr = std::ranges::find(compiled->entry_points, ShaderStage::Fragment,
                                 &EntryPoint::stage);

    if (itr == compiled->entry_points.end()) {
        UME_LOG_ERROR(
            Renderer,
            "could not find fragment entry point in post effect shader {}",
            shader_path.string());
        return {};
    }

    const uint32_t params_size = postEffectParamsSize(*compiled);

    GraphicsPipelineHandle pipeline =
        backend_->createGraphicsPipeline({.shader = shader});
    if (!pipeline) return {};

    return post_effects_.insert({
        .shader = shader,
        .pipeline = pipeline,
        .params_size = params_size,
        .source = shader_path,
    });
}

bool Renderer::reloadPostEffect(PostEffectHandle handle) {
    PostEffect *effect = post_effects_.get(handle);
    if (effect == nullptr) return false;
    std::optional<CompiledShader> compiled = compiler_->compile(effect->source);
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

    GraphicsPipelineHandle pipeline =
        backend_->createGraphicsPipeline({.shader = shader});
    if (!pipeline) {
        backend_->destroyShader(shader);
        return false;
    }

    backend_->destroyGraphicsPipeline(effect->pipeline);
    backend_->destroyShader(effect->shader);

    effect->shader = shader;
    effect->pipeline = pipeline;
    effect->params_size = postEffectParamsSize(*compiled);
    UME_LOG_INFO(Renderer, "reloaded '{}'", effect->source.string());
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
}

void Renderer::submitPostEffect(PostEffectHandle handle,
                                std::span<const std::byte> params) {
    PostEffect *effect = post_effects_.get(handle);
    if (effect == nullptr) {
        UME_LOG_WARN(Renderer, "invalid post effect handle submitted {}",
                     handle.id);
        return;
    }
    if (params.size() != effect->params_size) {
        UME_LOG_WARN(Renderer, "post effect {} expects {} param bytes, got {}",
                     handle.id, effect->params_size, params.size());
        return;
    }
    const size_t offset = post_param_arena_.size();
    post_param_arena_.insert(post_param_arena_.end(), params.begin(),
                             params.end());
    post_submissions_.push_back({.pipeline = effect->pipeline,
                                 .param_offset = offset,
                                 .param_size = params.size()});
}

void Renderer::render() {
    if (pixel_width_ == 0 || pixel_height_ == 0) return;

    std::array<float, 1000> readback{};
    backend_->readBuffer(output_, 0,
                         std::as_writable_bytes(std::span{readback}));

    const glm::mat4 projection =
        perspectiveReverseZ(camera_state_.fov_y, aspect_, camera_state_.z_near);

    const auto view_rotation =
        glm::mat4(glm::transpose(camera_state_.orientation));

    backend_->beginFrame();

    backend_->beginComputePass();
    const std::array<BufferBinding, 2> bindings = {
        BufferBinding{.slot = 0, .buffer = input_},
        BufferBinding{.slot = 1, .buffer = output_},
    };

    backend_->dispatch({.pipeline = compute_test_pipeline_,
                        .work_size = {1000, 1, 1},
                        .bindings = {.buffers = bindings}});

    backend_->endComputePass();

    backend_->beginScenePass();

    for (const auto &next : submissions_) {
        const auto relative =
            glm::vec3(next.world_position - camera_state_.position);

        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), relative) * next.local_transform;

        Mesh mesh = next.mesh;
        DrawUniforms uniforms{.model_view_projection =
                                  projection * view_rotation * model,
                              .normal = glm::transpose(glm::inverse(model)),
                              .base_color = next.base_color};

        const std::array<BufferBinding, 1> vertex_buffers = {
            BufferBinding{
                .slot = vertices_slot_,
                .buffer = mesh.vertex_buffer,
                .offset = 0,
            },
        };

        backend_->draw({
            .pipeline = default_pipeline_,
            .index_buffer = mesh.index_buffer,
            .index_count = mesh.index_count,
            .index_type = mesh.index_type,
            .vertex_bindings{
                .buffers = vertex_buffers,
                .params = std::as_bytes(std::span(&uniforms, 1)),
                .params_slot = draw_uniforms_slot_,
            },

        });
    }
    submissions_.clear();

    const PostFrameUniforms frame_uniforms{
        .inverse_projection = glm::inverse(projection),
        .camera_to_world = glm::mat4(camera_state_.orientation),
        .resolution = glm::vec2(pixel_width_, pixel_height_),
        .z_near = camera_state_.z_near,
        .fov_y = camera_state_.fov_y,
        .aspect = aspect_,
    };

    post_passes_.clear();
    for (const auto &s : post_submissions_) {
        post_passes_.push_back(
            {.pipeline = s.pipeline,
             .params = std::span(post_param_arena_)
                           .subspan(s.param_offset, s.param_size)});
    }

    backend_->postProcess(
        {.frame_uniforms = std::as_bytes(std::span(&frame_uniforms, 1)),
         .passes = post_passes_});

    post_submissions_.clear();
    post_param_arena_.clear();

    backend_->endFrame();
}

void Renderer::setCamera(const CameraState &camera_state) {
    camera_state_ = camera_state;
}

} // namespace ume