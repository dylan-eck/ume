#pragma once

#include "../../renderer/renderer_backend.hpp"
#include "vulkan_context.hpp"

namespace ume {
class VulkanRenderer : public RendererBackend {
public:
    void init() override;
    void beginFrame() override;
    void endFrame() override;

private:
    VulkanContext context_;
};
} // namespace ume