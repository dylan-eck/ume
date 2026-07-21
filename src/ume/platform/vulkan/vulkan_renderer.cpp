#include "vulkan_renderer.hpp"

namespace ume {

void VulkanRenderer::init() {}

void VulkanRenderer::beginFrame() {}

void VulkanRenderer::endFrame() {}

std::unique_ptr<RendererBackend> createRendererBackend() {
    return std::make_unique<VulkanRenderer>();
}
} // namespace ume