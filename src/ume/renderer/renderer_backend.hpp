#pragma once

#include <memory>

namespace ume {

class RendererBackend {
public:
    virtual ~RendererBackend() = default;

    virtual void init() = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
};

std::unique_ptr<RendererBackend> createRendererBackend();
} // namespace ume