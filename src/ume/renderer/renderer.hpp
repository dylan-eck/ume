#pragma once

#include "renderer_backend.hpp"

#include <memory>

namespace ume {

class Renderer {
public:
    Renderer();

    void init();
    void shutdown();

    void beginFrame();
    void endFrame();

private:
    std::unique_ptr<RendererBackend> backend_;
};
} // namespace ume