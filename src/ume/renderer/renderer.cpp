#include "renderer.hpp"
#include "ume/core/logger.hpp"

namespace ume {

struct DrawUniforms {
    glm::mat4 model_view_projection;
};

Renderer::Renderer(void *native_window_handle)
    : backend_(createRendererBackend(native_window_handle)) {}

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

void Renderer::submit(MeshHandle handle, const glm::mat4 &transform) {
    Mesh *mesh = meshes_.get(handle);
    if (mesh == nullptr) {
        UME_LOG_WARN(Renderer, "invalid mesh handle submitted {}", handle.id);
        return;
    }

    submissions_.push_back({.mesh = *mesh, .transform = transform});
}

void Renderer::render() {

    auto view =
        glm::lookAt(glm::vec3(0.0f, 4.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));
    auto projection =
        glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);

    backend_->beginFrame();

    for (const auto &next : submissions_) {
        Mesh mesh = next.mesh;
        DrawUniforms uniforms{.model_view_projection =
                                  projection * view * next.transform};

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