#include "renderer.hpp"

namespace ume {
Renderer::Renderer(void *native_window_handle)
    : backend_(createRendererBackend(native_window_handle)) {}

void Renderer::beginFrame() { backend_->beginFrame(); }

void Renderer::endFrame() { backend_->endFrame(); }

} // namespace ume