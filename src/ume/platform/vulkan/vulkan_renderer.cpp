#include "vulkan_renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/platform/window.hpp"

#include <VkBootstrap.h>
#include "SDL3/SDL_vulkan.h"
// #include "battery/embed.hpp"

namespace ume {

VulkanRenderer::VulkanRenderer(const Window &window)
    : context_(reinterpret_cast<PFN_vkGetInstanceProcAddr>(
          SDL_Vulkan_GetVkGetInstanceProcAddr())) {
    initVulkan(window);

    UME_LOG_INFO(Renderer, "initialized renderer");
}

void VulkanRenderer::beginFrame() {
    auto fence_result = device_.waitForFences(
        *render_finished_fences_[frame_index_], vk::True, UINT64_MAX);
    if (fence_result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }
    device_.resetFences(*render_finished_fences_[frame_index_]);

    auto [result, image_index] = swapchain_.acquireNextImage(
        UINT64_MAX, *image_available_semaphores_[frame_index_], nullptr);

    command_buffers_[frame_index_].reset();
    recordCommandBuffer(image_index);

    vk::PipelineStageFlags wait_mask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);

    const vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*image_available_semaphores_[frame_index_],
        .pWaitDstStageMask = &wait_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*command_buffers_[frame_index_],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*render_finished_semaphores_[image_index],
    };

    queue_.submit(submit_info, *render_finished_fences_[frame_index_]);

    vk::PresentInfoKHR present_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*render_finished_semaphores_[image_index],
        .swapchainCount = 1,
        .pSwapchains = &*swapchain_,
        .pImageIndices = &image_index,
    };

    result = queue_.presentKHR(present_info);

    frame_index_ = (frame_index_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::draw(const DrawCommand &cmd) {}

void VulkanRenderer::endFrame() {}

BufferHandle
VulkanRenderer::createBuffer(const BufferDescription &buffer_description) {
    return {};
}

void VulkanRenderer::destroyBuffer(BufferHandle handle) {}

void VulkanRenderer::initVulkan(const Window &window) {
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

    UME_LOG_INFO(Renderer, "using vulkan: {}.{}.{}",
                 VK_VERSION_MAJOR(inst_ret->api_version),
                 VK_VERSION_MINOR(inst_ret->api_version),
                 VK_VERSION_PATCH(inst_ret->api_version));

    vkb::Instance vkb_inst = inst_ret.value();
    instance_ = vk::raii::Instance(context_, vkb_inst);
    debug_messenger_ =
        vk::raii::DebugUtilsMessengerEXT(instance_, vkb_inst.debug_messenger);

    VkSurfaceKHR surface = window.createVulkanSurface(vkb_inst.instance);
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
                         {.synchronization2 = 1u, .dynamicRendering = 1u},
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
    queue_ = vk::raii::Queue(device_, queue_family_index_, 0);

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

    swapchain_extent_ = vkb_swapchain.extent;

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

    // std::vector<uint8_t> shader_source =
    //     b::embed<"generated/src/ume/renderer/shaders/default.slang.spv">()
    //         .vec();
    // vk::raii::ShaderModule shader_module = createShaderModule(shader_source);

    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanRenderer::createCommandPool() {
    vk::CommandPoolCreateInfo create_info{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queue_family_index_,
    };

    command_pool_ = vk::raii::CommandPool(device_, create_info);
}

void VulkanRenderer::createCommandBuffers() {
    vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = command_pool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    command_buffers_ = std::move(vk::raii::CommandBuffers(device_, alloc_info));
}

void VulkanRenderer::createSyncObjects() {
    assert(render_finished_semaphores_.empty() &&
           image_available_semaphores_.empty() &&
           render_finished_fences_.empty());

    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        render_finished_semaphores_.emplace_back(device_,
                                                 vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        image_available_semaphores_.emplace_back(device_,
                                                 vk::SemaphoreCreateInfo());
        render_finished_fences_.emplace_back(
            device_,
            vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void VulkanRenderer::recordCommandBuffer(uint32_t image_index) {
    command_buffers_[frame_index_].begin({});

    transitionImageLayout(image_index, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eColorAttachmentOptimal, {},
                          vk::AccessFlagBits2::eColorAttachmentWrite,
                          vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                          vk::PipelineStageFlagBits2::eColorAttachmentOutput);

    float v = (0.5f * std::sin(time_)) + 0.5f;

    vk::ClearValue clear_color = vk::ClearColorValue{v, 0.0f, v, 1.0f};

    time_ += 0.01;

    vk::RenderingAttachmentInfo attachment_info{
        .imageView = swapchain_image_views_[image_index],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clear_color,
    };

    vk::RenderingInfo render_info{
        .renderArea = {.offset = {.x = 0, .y = 0}, .extent = swapchain_extent_},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_info,
    };

    command_buffers_[frame_index_].beginRendering(render_info);

    command_buffers_[frame_index_].setViewport(
        0,
        vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchain_extent_.width),
                     static_cast<float>(swapchain_extent_.height), 0.0f, 1.0f));
    command_buffers_[frame_index_].setScissor(
        0, vk::Rect2D(vk::Offset2D(0, 0), swapchain_extent_));

    command_buffers_[frame_index_].endRendering();

    transitionImageLayout(image_index, vk::ImageLayout::eColorAttachmentOptimal,
                          vk::ImageLayout::ePresentSrcKHR,
                          vk::AccessFlagBits2::eColorAttachmentWrite, {},
                          vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                          vk::PipelineStageFlagBits2::eBottomOfPipe);

    command_buffers_[frame_index_].end();
}

void VulkanRenderer::transitionImageLayout(
    uint32_t image_index, vk::ImageLayout old_layout,
    vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask) {

    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_images_[image_index],
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    vk::DependencyInfo dependency_info{.dependencyFlags = {},
                                       .imageMemoryBarrierCount = 1,
                                       .pImageMemoryBarriers = &barrier};

    command_buffers_[frame_index_].pipelineBarrier2(dependency_info);
}

vk::raii::ShaderModule
VulkanRenderer::createShaderModule(std::vector<uint8_t> &shader_source) {

    vk::ShaderModuleCreateInfo create_info{
        .codeSize = static_cast<uint32_t>(shader_source.size()),
        .pCode = reinterpret_cast<uint32_t *>(shader_source.data())};

    vk::raii::ShaderModule module(device_, create_info);
    return module;
}

std::unique_ptr<RendererBackend> createRendererBackend(const Window &window) {
    return std::make_unique<VulkanRenderer>(window);
}
} // namespace ume