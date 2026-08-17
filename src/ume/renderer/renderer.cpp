#include "renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/platform/window.hpp"

namespace ume {

struct DrawUniforms {
    glm::mat4 model_view_projection;
};

Renderer::Renderer(const Window &window)
    : backend_(createRendererBackend(window)),
      aspect_(static_cast<float>(window.getPixelWidth()) /
              static_cast<float>(window.getPixelHeight())) {}

Renderer::~Renderer() {}

MeshHandle Renderer::createMesh(const MeshDescription &desc) {
    BufferHandle vertex_buffer =
        backend_->createBuffer({.size = desc.vertices.size_bytes(),
                                .initial_data = desc.vertices.data()});
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

void Renderer::setCamera(const CameraState &camera_state) {
    camera_state_ = camera_state;
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
                                  projection * view_rotation * model};

        backend_->draw({
            .vertex_buffer = mesh.vertex_buffer,
            .index_buffer = mesh.index_buffer,
            .index_count = mesh.index_count,
            .index_type = mesh.index_type,
            .push_constants = std::as_bytes(std::span(&uniforms, 1)),
        });
    }
    submissions_.clear();

    backend_->endFrame();
}

} // namespace ume