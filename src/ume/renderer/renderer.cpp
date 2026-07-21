#include "renderer.hpp"

namespace ume {
void Renderer::init() {}

void Renderer::shutdown() {}

void Renderer::beginFrame() {}

void Renderer::endFrame() {}

Renderer::Renderer() { backend_ = createRendererBackend(); }
} // namespace ume