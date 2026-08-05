#pragma once

#include "ume/renderer/renderer_backend.hpp"
#include "ume/renderer/resource_pool.hpp"

#include <SDL3/SDL.h>

// NOLINTBEGIN (forward declaring metal types)
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
// NOLINTEND

namespace ume {

struct MetalBuffer {
    MTL::Buffer *buffer;
    size_t size = 0;
};

class MetalRenderer : public RendererBackend {
public:
    explicit MetalRenderer(void *native_window_handle, uint32_t pixel_width,
                           uint32_t pixel_height);

    ~MetalRenderer();

    MetalRenderer(const MetalRenderer &) = delete;
    MetalRenderer &operator=(const MetalRenderer &) = delete;

    MetalRenderer(MetalRenderer &&) = delete;
    MetalRenderer &operator=(MetalRenderer &&) = delete;

    void beginFrame() override;
    void draw(const DrawCommand &cmd) override;
    void endFrame() override;

    BufferHandle
    createBuffer(const BufferDescription &buffer_description) override;
    void destroyBuffer(BufferHandle handle) override;

private:
    SDL_MetalView metal_view_;
    CA::MetalLayer *layer_ = nullptr;
    MTL::Device *device_ = nullptr;
    MTL::CommandQueue *command_queue_ = nullptr;
    MTL::RenderPipelineState *pipeline_state_ = nullptr;

    NS::AutoreleasePool *frame_pool_ = nullptr;
    CA::MetalDrawable *drawable_ = nullptr;
    MTL::CommandBuffer *command_buffer_ = nullptr;
    MTL::RenderCommandEncoder *encoder_ = nullptr;

    MTL::Texture *depth_texture_;
    MTL::DepthStencilState *depth_state_;

    ResourcePool<MetalBuffer, BufferHandle> buffers_;
};
} // namespace ume