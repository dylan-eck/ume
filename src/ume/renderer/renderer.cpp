#include "renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/platform/window.hpp"
#include "ume/renderer/shader_compiler.hpp"

namespace ume {

// TODO: better naming?
struct DrawUniforms {
    glm::mat4 model_view_projection;
    glm::mat4 normal; // normal matrix
};

// TODO: reorder this?
struct PostFrameUniforms {
    glm::vec2 resolution;
    float z_near;
    float fov_y;
    float aspect;
    float time;
    float _pad[2];
};
static_assert(sizeof(PostFrameUniforms) == 32);

Renderer::Renderer(const Window &window)
    : pixel_width_(window.getPixelWidth()),
      pixel_height_(window.getPixelHeight()),
      aspect_(static_cast<float>(pixel_width_) /
              static_cast<float>(pixel_height_)),
      backend_(createRendererBackend(window)),
      compiler_(std::make_unique<ShaderCompiler>(
          backend_->shaderTarget(),
          std::filesystem::path(UME_SOURCE_DIR) / "include")) {}

Renderer::~Renderer() = default;

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
                                .initial_data = vertices.data()});
    if (!vertex_buffer) {
        return {};
    }

    BufferHandle index_buffer =
        backend_->createBuffer({.size = desc.indices.size_bytes(),
                                .initial_data = desc.indices.data()});
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
                            .local_transform = local_transform});
}

PostEffectHandle
Renderer::createPostEffect(const std::filesystem::path &shader) {
    std::optional<CompiledShader> compiled =
        compiler_->compilePostEffect(shader);
    if (!compiled) {
        return {};
    }

    PipelineHandle pipeline =
        backend_->createPipeline({.shader = compiled->code});
    if (!pipeline) {
        return {};
    }

    return post_effects_.insert({.pipeline = pipeline,
                                 .params_size = compiled->params_size,
                                 .source = shader});
}

bool Renderer::reloadPostEffect(PostEffectHandle handle) {
    PostEffect *effect = post_effects_.get(handle);
    if (effect == nullptr) {
        return false;
    }
    std::optional<CompiledShader> compiled =
        compiler_->compilePostEffect(effect->source);
    if (!compiled) {
        return false; // keep the last good pipeline; the error is already
                      // logged
    }
    PipelineHandle pipeline =
        backend_->createPipeline({.shader = compiled->code});
    if (!pipeline) {
        return false;
    }
    backend_->destroyPipeline(effect->pipeline);
    effect->pipeline = pipeline;
    effect->params_size = compiled->params_size;
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
    backend_->destroyPipeline(effect->pipeline);
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
    const glm::mat4 projection =
        perspectiveReverseZ(camera_state_.fov_y, aspect_, camera_state_.z_near);

    const auto view_rotation =
        glm::mat4(glm::transpose(camera_state_.orientation));

    backend_->beginFrame();

    for (const auto &next : submissions_) {
        const auto relative =
            glm::vec3(next.world_position - camera_state_.position);

        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), relative) * next.local_transform;

        Mesh mesh = next.mesh;
        DrawUniforms uniforms{.model_view_projection =
                                  projection * view_rotation * model,
                              .normal = glm::transpose(glm::inverse(model))};

        backend_->draw({
            .vertex_buffer = mesh.vertex_buffer,
            .index_buffer = mesh.index_buffer,
            .index_count = mesh.index_count,
            .index_type = mesh.index_type,
            .push_constants = std::as_bytes(std::span(&uniforms, 1)),
        });
    }
    submissions_.clear();

    const PostFrameUniforms frame_uniforms{
        .resolution = glm::vec2(pixel_width_, pixel_height_),
        .z_near = camera_state_.z_near,
        .fov_y = camera_state_.fov_y,
        .aspect = aspect_,
        .time = std::chrono::duration<float>(std::chrono::steady_clock::now() -
                                             start_time_)
                    .count()};

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