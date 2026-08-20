#pragma once

#include "ume/renderer/renderer_backend.hpp"
#include "ume/core/resource_pool.hpp"
#include "ume/platform/window.hpp"

#include <Foundation/NSSharedPtr.hpp>

// NOLINTBEGIN(readability-identifier-naming)
// forward declared metal types
namespace NS {
class Error;
class AutoreleasePool;
} // namespace NS

namespace CA {
class MetalLayer;
class MetalDrawable;
} // namespace CA

namespace MTL {
class Device;
class CommandQueue;
class CommandBuffer;
class RenderCommandEncoder;
class RenderPipelineState;
class Buffer;
class Texture;
class DepthStencilState;
} // namespace MTL
// NOLINTEND(readability-identifier-naming)

namespace ume {

struct MetalBuffer {
    NS::SharedPtr<MTL::Buffer> buffer;
    size_t size = 0;
};

class MetalRenderer : public RendererBackend {
public:
    explicit MetalRenderer(MetalSurface surface, uint32_t pixel_width,
                           uint32_t pixel_height);

    ~MetalRenderer() override = default;

    MetalRenderer(const MetalRenderer &) = delete;
    MetalRenderer &operator=(const MetalRenderer &) = delete;

    MetalRenderer(MetalRenderer &&) = delete;
    MetalRenderer &operator=(MetalRenderer &&) = delete;

    void beginFrame() override;
    void draw(const DrawCommand &cmd) override;
    void endFrame() override;

    BufferHandle createBuffer(const BufferDescription &desc) override;
    void destroyBuffer(BufferHandle handle) override;

private:
    MetalSurface surface_;
    CA::MetalLayer *layer_ = nullptr;

    NS::SharedPtr<MTL::Device> device_ = nullptr;
    NS::SharedPtr<MTL::CommandQueue> command_queue_ = nullptr;
    NS::SharedPtr<MTL::RenderPipelineState> pipeline_state_ = nullptr;
    NS::SharedPtr<MTL::Texture> depth_texture_;
    NS::SharedPtr<MTL::DepthStencilState> depth_state_;

    NS::SharedPtr<NS::AutoreleasePool> frame_pool_ = nullptr;
    CA::MetalDrawable *drawable_ = nullptr;
    MTL::CommandBuffer *command_buffer_ = nullptr;
    MTL::RenderCommandEncoder *encoder_ = nullptr;

    ResourcePool<MetalBuffer, BufferHandle> buffers_;
};
} // namespace ume