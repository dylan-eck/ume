#pragma once

#include "renderer_backend.hpp"

#include <memory>

namespace ume {

class Renderer {
public:
    explicit Renderer(void *native_window_handle);

    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;

    void beginFrame();
    void endFrame();

private:
    std::unique_ptr<RendererBackend> backend_;

    BufferHandle vertex_buffer_;
};
} // namespace ume