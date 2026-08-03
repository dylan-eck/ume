#include "renderer.hpp"
#include "ume/core/logger.hpp"

namespace ume {

struct DrawUniforms {
    glm::mat4 model_view_projection;
};

Renderer::Renderer(void *native_window_handle, uint32_t pixel_width,
                   uint32_t pixel_height)
    : backend_(createRendererBackend(native_window_handle, pixel_width,
                                     pixel_height)) {

    aspect_ = (float)pixel_width / (float)pixel_height;
}

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
    Mesh *mesh = meshes_.retire(handle);
    if (mesh == nullptr) {
        UME_LOG_WARN(Renderer, "attempted to destroy stale mesh handle: {}",
                     handle.id);
        return;
    }

    backend_->destroyBuffer(mesh->vertex_buffer);
    backend_->destroyBuffer(mesh->index_buffer);

    meshes_.reclaim(handle);
}

void Renderer::setCamera(const glm::vec3 &position, const glm::vec3 &target,
                         float fov_y) {
    view_ = glm::lookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
    projection_ = glm::perspective(fov_y, aspect_, 0.1f, 100.0f);
}

void Renderer::submit(MeshHandle handle, const glm::mat4 &transform) {
    Mesh *mesh = meshes_.get(handle);
    if (mesh == nullptr) {
        UME_LOG_WARN(Renderer, "invalid mesh handle submitted {}", handle.id);
        return;
    }

    submissions_.push_back({.mesh = *mesh, .transform = transform});
}

void Renderer::render() {

    backend_->beginFrame();

    for (const auto &next : submissions_) {
        Mesh mesh = next.mesh;
        DrawUniforms uniforms{.model_view_projection =
                                  projection_ * view_ * next.transform};

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