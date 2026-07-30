#pragma once

#include "../../renderer/renderer_backend.hpp"

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#include <vulkan/vulkan_raii.hpp>

namespace ume {
class VulkanRenderer : public RendererBackend {
public:
    explicit VulkanRenderer(void *native_window_handle);

    VulkanRenderer(const VulkanRenderer &) = delete;
    VulkanRenderer &operator=(const VulkanRenderer &) = delete;

    VulkanRenderer(VulkanRenderer &&) = delete;
    VulkanRenderer &operator=(VulkanRenderer &&) = delete;

    void beginFrame() override;
    void endFrame() override;

private:
    vk::raii::Context context_;
    vk::raii::Instance instance_ = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;
    vk::raii::SurfaceKHR surface_ = nullptr;
    vk::raii::Device device_ = nullptr;
    vk::raii::Queue queue_ = nullptr;

    VkFormat swapchain_format_ = VK_FORMAT_B8G8R8A8_UNORM;
    vk::raii::SwapchainKHR swapchain_ = nullptr;
    std::vector<vk::Image> swapchain_images_;
    std::vector<vk::raii::ImageView> swapchain_image_views_;

    void initVulkan(void *native_window_handle);

    [[nodiscard]] vk::raii::ShaderModule
    createShaderModule(std::vector<uint8_t> &shader_source);
};
} // namespace ume