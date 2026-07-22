#pragma once

#include "../../renderer/renderer_backend.hpp"
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
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
    vk::raii::Instance instance_{nullptr};

    void createInstance();
};
} // namespace ume