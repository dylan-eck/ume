#pragma once

#include <vulkan/vulkan.h>

namespace ume {
class VulkanContext {
public:
    VulkanContext();

private:
    VkInstance instance_;
};
} // namespace ume