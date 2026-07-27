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
    vkb::InstanceBuilder builder(reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        SDL_Vulkan_GetVkGetInstanceProcAddr()));

    auto inst_ret = builder.set_app_name("Test Application")
                        .set_app_version(VK_MAKE_VERSION(0, 1, 1))
                        .set_engine_name("Ume Engine")
                        .set_engine_version(VK_MAKE_VERSION(0, 1, 0))
                        .require_api_version(1, 4, 0)
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .build();

    if (!inst_ret) {
        throw std::runtime_error("failed to create vulkan instance: " +
                                 inst_ret.error().message());
    }

    vkb::Instance vkb_inst = inst_ret.value();
    instance_ = vk::raii::Instance(context_, vkb_inst);
    debug_messenger_ =
        vk::raii::DebugUtilsMessengerEXT(instance_, vkb_inst.debug_messenger);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    auto *sdl_window = (SDL_Window *)native_window_handle;
    if (!SDL_Vulkan_CreateSurface(sdl_window, vkb_inst.instance, nullptr,
                                  &surface)) {
        const char *error = SDL_GetError();

        throw std::runtime_error(error);
    }

    // surface_ = vk::raii::SurfaceKHR(instance_, surface);

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