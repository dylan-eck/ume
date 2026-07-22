#pragma once

#include "renderer_backend.hpp"

#include <memory>

namespace ume {

class Renderer {
public:
    explicit Renderer(void *native_window_handle);

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void beginFrame();
    void endFrame();

private:
    std::unique_ptr<RendererBackend> backend_;
};
} // namespace ume