#include "vulkan_renderer.hpp"
#include "../../core/logger.hpp"

#include <VkBootstrap.h>
#include "SDL3/SDL_vulkan.h"

namespace ume {

VulkanRenderer::VulkanRenderer(void *native_window_handle)
    : context_(reinterpret_cast<PFN_vkGetInstanceProcAddr>(
          SDL_Vulkan_GetVkGetInstanceProcAddr())) {
    initVulkan(native_window_handle);

    UME_LOG_INFO(Renderer, "initialized renderer");
}

void VulkanRenderer::beginFrame() {}

void VulkanRenderer::endFrame() {}

void VulkanRenderer::initVulkan(void *native_window_handle) {

    constexpr vk::ApplicationInfo kAppInfo{
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14};

    uint32_t count;
    const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    std::vector<const char *> required_extensions(extensions,
                                                  extensions + count);
    required_extensions.push_back(
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    vk::InstanceCreateInfo create_info{
        .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        .pApplicationInfo = &kAppInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data()};

    instance_ = vk::raii::Instance(context_, create_info);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    auto *sdl_window = (SDL_Window *)native_window_handle;
    if (!SDL_Vulkan_CreateSurface(sdl_window, *instance_, nullptr, &surface)) {
        const char *error = SDL_GetError();

        throw std::runtime_error(error);
    }

    // vkb::InstanceBuilder builder;
    // auto inst_ret =
    //     builder.set_app_name("Test Application")
    //         .request_validation_layers()
    //         .use_default_debug_messenger()
    //         .require_api_version(1, 3, 0)
    //         .enable_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
    //         .build();

    // if (!inst_ret) {
    //     throw std::runtime_error("failed to create vulkan instance: " +
    //                              inst_ret.error().message());
    // }

    // vkb::Instance vkb_inst = inst_ret.value();

    // vkb::PhysicalDeviceSelector selector{vkb_inst};
    // auto phys_ret = selector.set_surface(surface).select();
    // if (!phys_ret) {
    //     throw std::runtime_error("failed to select physical device");
    // }

    // vkb::DeviceBuilder device_builder{phys_ret.value()};
    // auto dev_ret = device_builder.build();
    // if (!dev_ret) {
    //     throw std::runtime_error("failed to create logical device");
    // }
    // vkb::Device vkb_device = dev_ret.value();

    // VkDevice device = vkb_device.device;

    // auto graphics_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
    // if (!graphics_queue_ret) {
    //     throw std::runtime_error("no graphics queue found");
    // }
    // VkQueue graphics_queue = graphics_queue_ret.value();

    // vkb::SwapchainBuilder swapchain_builder{vkb_device};
    // auto swap_ret = swapchain_builder.build();
    // if (!swap_ret) {
    //     throw std::runtime_error("failed to create swapchain");
    // }
    // vkb::Swapchain vkb_swapchain = swap_ret.value();
}

std::unique_ptr<RendererBackend>
createRendererBackend(void *native_window_handle) {
    return std::make_unique<VulkanRenderer>(native_window_handle);
}
} // namespace ume