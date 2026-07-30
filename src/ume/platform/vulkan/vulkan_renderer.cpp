#include "vulkan_renderer.hpp"
#include "../../core/logger.hpp"

#include <VkBootstrap.h>
#include "SDL3/SDL_vulkan.h"
#include "battery/embed.hpp"

#include <iostream>
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

        throw std::runtime_error("failed to create surface: " +
                                 std::string(error));
    }

    surface_ = vk::raii::SurfaceKHR(instance_, surface);

    vk::raii::PhysicalDevice physical_device =
        instance_.enumeratePhysicalDevices().front();

    float queue_priority = 1.0;
    auto queue_create_info =
        vk::DeviceQueueCreateInfo{.queueFamilyIndex = 0,
                                  .queueCount = 1,
                                  .pQueuePriorities = &queue_priority};

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        feature_chain = {{},
                         {.shaderDrawParameters = 1u},
                         {.dynamicRendering = 1u},
                         {.extendedDynamicState = 1u}};

    std::vector<const char *> required_device_extension = {
        vk::KHRSwapchainExtensionName, "VK_KHR_portability_subset"};

    vk::DeviceCreateInfo device_create_info{
        .pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount =
            static_cast<uint32_t>(required_device_extension.size()),
        .ppEnabledExtensionNames = required_device_extension.data()};

    device_ = vk::raii::Device(physical_device, device_create_info);
    queue_ = vk::raii::Queue(device_, 0, 0);

    vkb::SwapchainBuilder swapchain_builder(*physical_device, *device_,
                                            *surface_);
    vkb::Swapchain vkb_swapchain =
        swapchain_builder
            .set_desired_format(VkSurfaceFormatKHR{
                .format = swapchain_format_,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(1280, 720)
            .build()
            .value();

    swapchain_ = vk::raii::SwapchainKHR(device_, vkb_swapchain.swapchain);
    swapchain_images_ = swapchain_.getImages();

    vk::ImageViewCreateInfo view_create_info{
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format(swapchain_format_),
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    for (auto &image : swapchain_images_) {
        view_create_info.image = image;
        swapchain_image_views_.emplace_back(device_, view_create_info);
    }

    size_t data_length =
        b::embed<"generated/src/ume/renderer/shaders/triangle.slang.spv">()
            .length();

    const char *data =
        b::embed<"generated/src/ume/renderer/shaders/triangle.slang.spv">()
            .data();

    for (size_t i = 0; i < data_length; i++) {
        if (i > 0 && (i % 8 == 0)) {
            std::cout << "\n";
        }
        std::print("0x{:02X} ", (unsigned char)data[i]);
    }
    std::cout << "\n";
}

std::unique_ptr<RendererBackend>
createRendererBackend(void *native_window_handle) {
    return std::make_unique<VulkanRenderer>(native_window_handle);
}
} // namespace ume