#pragma once

#include <memory>

namespace ume {

class RendererBackend {
public:
    virtual ~RendererBackend() = default;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
};

std::unique_ptr<RendererBackend>
createRendererBackend(void *native_window_handle);
} // namespace ume