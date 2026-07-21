#include "vulkan_renderer.hpp"
#include "../../core/logger.hpp"

namespace ume {

VulkanRenderer::VulkanRenderer(void *native_window_handle) {
    UME_LOG_INFO(Renderer, "initialized renderer");
}

void VulkanRenderer::beginFrame() {}

void VulkanRenderer::endFrame() {}

std::unique_ptr<RendererBackend>
createRendererBackend(void *native_window_handle) {
    return std::make_unique<VulkanRenderer>(native_window_handle);
}
} // namespace ume