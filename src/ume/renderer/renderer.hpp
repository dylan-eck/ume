#pragma once

#include "renderer_backend.hpp"

#include <memory>

namespace ume {

class Renderer {
public:
    explicit Renderer(void *native_window_handle);

    void beginFrame();
    void endFrame();

private:
    std::unique_ptr<RendererBackend> backend_;
};
} // namespace ume