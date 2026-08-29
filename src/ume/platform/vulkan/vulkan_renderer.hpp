#pragma once

#include "ume/renderer/renderer_backend.hpp"

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0
#include <vulkan/vulkan_raii.hpp>

namespace ume {
class VulkanRenderer : public RendererBackend {
public:
    explicit VulkanRenderer(const Window &window);

    VulkanRenderer(const VulkanRenderer &) = delete;
    VulkanRenderer &operator=(const VulkanRenderer &) = delete;

    VulkanRenderer(VulkanRenderer &&) = delete;
    VulkanRenderer &operator=(VulkanRenderer &&) = delete;

    void beginFrame() override;
    void draw(const DrawCommand &cmd) override;
    void endFrame() override;

    BufferHandle
    createBuffer(const BufferDescription &buffer_description) override;
    void destroyBuffer(BufferHandle handle) override;

private:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t frame_index_ = 0;

    float time_ = 0;

    vk::raii::Context context_;
    vk::raii::Instance instance_ = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;
    vk::raii::SurfaceKHR surface_ = nullptr;
    vk::raii::Device device_ = nullptr;
    uint32_t queue_family_index_ = 0; // TODO: this should not be hard coded
    vk::raii::Queue queue_ = nullptr;

    vk::raii::CommandPool command_pool_ = nullptr;
    std::vector<vk::raii::CommandBuffer> command_buffers_;
    std::vector<vk::raii::Semaphore> image_available_semaphores_;
    std::vector<vk::raii::Semaphore> render_finished_semaphores_;
    std::vector<vk::raii::Fence> render_finished_fences_;

    VkFormat swapchain_format_ = VK_FORMAT_B8G8R8A8_UNORM;
    vk::raii::SwapchainKHR swapchain_ = nullptr;
    std::vector<vk::Image> swapchain_images_;
    std::vector<vk::raii::ImageView> swapchain_image_views_;
    vk::Extent2D swapchain_extent_;

    void initVulkan(const Window &window);
    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffer(uint32_t image_index);
    void createSyncObjects();

    void transitionImageLayout(uint32_t image_index, vk::ImageLayout old_layout,
                               vk::ImageLayout new_layout,
                               vk::AccessFlags2 src_access_mask,
                               vk::AccessFlags2 dst_access_mask,
                               vk::PipelineStageFlags2 src_stage_mask,
                               vk::PipelineStageFlags2 dst_stage_mask);

    [[nodiscard]] vk::raii::ShaderModule
    createShaderModule(std::vector<uint8_t> &shader_source);
};
} // namespace ume