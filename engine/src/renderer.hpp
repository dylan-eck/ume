#pragma once

#include "window.hpp"

namespace ume {
class Renderer {
public:
    explicit Renderer(Window &window);
    ~Renderer();

    void beginFrame();
    void endFrame();
};
} // namespace ume