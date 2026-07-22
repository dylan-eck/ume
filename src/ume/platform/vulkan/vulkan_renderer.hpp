#pragma once

#include "../../renderer/renderer_backend.hpp"
#include "vulkan_context.hpp"

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
    VulkanContext context_;
};
} // namespace ume