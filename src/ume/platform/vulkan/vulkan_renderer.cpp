#include "vulkan_renderer.hpp"
#include "../../core/logger.hpp"

namespace ume {

VulkanRenderer::VulkanRenderer(void *native_window_handle) {
    createInstance();

    UME_LOG_INFO(Renderer, "initialized renderer");
}

void VulkanRenderer::beginFrame() {}

void VulkanRenderer::endFrame() {}

void VulkanRenderer::createInstance() {
    constexpr vk::ApplicationInfo kAppInfo{
        .pApplicationName = "Test Application",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "Ume",
        .engineVersion = VK_MAKE_VERSION(0, 1, 1),
        .apiVersion = vk::ApiVersion14,
    };

    vk::InstanceCreateInfo instance_info{.pApplicationInfo = &kAppInfo};
}

std::unique_ptr<RendererBackend>
createRendererBackend(void *native_window_handle) {
    return std::make_unique<VulkanRenderer>(native_window_handle);
}
} // namespace ume