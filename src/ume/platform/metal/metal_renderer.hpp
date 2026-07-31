#pragma once

#include <ume/renderer/renderer_backend.hpp>

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
} // namespace MTL
// NOLINTEND

namespace ume {
class MetalRenderer : public RendererBackend {
public:
    explicit MetalRenderer(void *native_window_handle);

    ~MetalRenderer();

    MetalRenderer(const MetalRenderer &) = delete;
    MetalRenderer &operator=(const MetalRenderer &) = delete;

    MetalRenderer(MetalRenderer &&) = delete;
    MetalRenderer &operator=(MetalRenderer &&) = delete;

    void beginFrame() override;
    void endFrame() override;

private:
    SDL_MetalView metal_view_;
    CA::MetalLayer *layer_;
    MTL::Device *device_;
    MTL::CommandQueue *command_queue_;
    MTL::RenderPipelineState *pipeline_state_;

    NS::AutoreleasePool *frame_pool_;
    CA::MetalDrawable *drawable_;
    MTL::CommandBuffer *command_buffer_;
    MTL::RenderCommandEncoder *encoder_;
};
} // namespace ume