#include "renderer.hpp"
#include "ume/core/logger.hpp"

#include <array>

namespace ume {
struct Vertex {
    std::array<float, 4> position;
    std::array<float, 4> color;
};

constexpr std::array<Vertex, 3> kTriangleVertices = {{
    Vertex{.position{0.0f, -0.5f, 0.0f, 1.0f}, .color{1.0f, 0.0f, 0.0f, 1.0f}},
    Vertex{.position{0.5f, 0.5f, 0.0f, 1.0f}, .color{0.0f, 1.0f, 0.0f, 1.0f}},
    Vertex{.position{-0.5f, 0.5f, 0.0f, 1.0f}, .color{0.0f, 0.0f, 1.0f, 1.0f}},
}};

Renderer::Renderer(void *native_window_handle)
    : backend_(createRendererBackend(native_window_handle)) {

    vertex_buffer_ = backend_->createBuffer({
        .size = sizeof(kTriangleVertices),
        .initial_data = kTriangleVertices.data(),
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
    backend_->draw({.vertex_buffer = vertex_buffer_, .vertex_count = 3});
    backend_->endFrame();
}

} // namespace ume