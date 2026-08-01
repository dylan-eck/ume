#include "renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/renderer/primitives.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ume {

struct DrawUniforms {
    glm::mat4 model_view_projection;
};

Renderer::Renderer(void *native_window_handle)
    : backend_(createRendererBackend(native_window_handle)) {

    vertex_buffer_ = backend_->createBuffer({
        .size = sizeof(ume::primitives::kCubeVertices),
        .initial_data = ume::primitives::kCubeVertices.data(),
    });

    index_buffer_ = backend_->createBuffer({
        .size = sizeof(ume::primitives::kCubeIndices),
        .initial_data = ume::primitives::kCubeIndices.data(),
    });

    UME_LOG_INFO(Renderer, "created buffer resource {}", vertex_buffer_.id);
}

Renderer::~Renderer() {
    if (vertex_buffer_) {
        backend_->destroyBuffer(vertex_buffer_);
    }
}

void Renderer::beginFrame() { backend_->beginFrame(); }

void Renderer::endFrame() {

    DrawUniforms uniforms{.model_view_projection =
                              glm::scale(glm::mat4(1.0f), glm::vec3(0.5f))};

    backend_->draw({.vertex_buffer = vertex_buffer_,
                    .vertex_count = 3,
                    .index_buffer = index_buffer_,
                    .index_count = 36,
                    .index_type = Indextype::UInt16,
                    .push_constants = std::as_bytes(std::span(&uniforms, 1))});
    backend_->endFrame();
}

} // namespace ume