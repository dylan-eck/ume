#include "renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/renderer/primitives.hpp"

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

    model_matrix_ = glm::mat4(1.0f);

    UME_LOG_INFO(Renderer, "created buffer resource {}", vertex_buffer_.id);
}

Renderer::~Renderer() {
    if (vertex_buffer_) {
        backend_->destroyBuffer(vertex_buffer_);
    }
}

void Renderer::beginFrame() { backend_->beginFrame(); }

void Renderer::endFrame() {

    model_matrix_ =
        glm::rotate(model_matrix_, 0.005f, glm::vec3(0.0f, 1.0f, 0.0f));

    auto view =
        glm::lookAt(glm::vec3(0.0f, 4.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));
    auto projection =
        glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);

    DrawUniforms uniforms{.model_view_projection =
                              projection * view * model_matrix_};

    backend_->draw({.vertex_buffer = vertex_buffer_,
                    .vertex_count = 3,
                    .index_buffer = index_buffer_,
                    .index_count = 36,
                    .index_type = Indextype::UInt16,
                    .push_constants = std::as_bytes(std::span(&uniforms, 1))});
    backend_->endFrame();
}

} // namespace ume