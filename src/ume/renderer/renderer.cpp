#include "renderer.hpp"

namespace ume {
Renderer::Renderer(void *native_window_handle)
    : backend_(createRendererBackend(native_window_handle)) {}

void Renderer::beginFrame() {}

void Renderer::endFrame() {}

} // namespace ume