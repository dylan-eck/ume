#pragma once

#include <memory>

namespace ume {

class RendererBackend {
public:
    RendererBackend() = default;
    virtual ~RendererBackend() = default;

    RendererBackend(const RendererBackend &) = delete;
    RendererBackend &operator=(const RendererBackend &) = delete;

    RendererBackend(RendererBackend &&) = delete;
    RendererBackend &operator=(RendererBackend &&) = delete;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
};

std::unique_ptr<RendererBackend>
createRendererBackend(void *native_window_handle);
} // namespace ume