#pragma once

#include "ume/renderer/renderer_backend.hpp"
#include "ume/core/resource_pool.hpp"
#include "ume/platform/window.hpp"

#include <Foundation/NSSharedPtr.hpp>

#include <array>

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
class SamplerState;
class Library;
class BlitCommandEncoder;
} // namespace MTL
// NOLINTEND(readability-identifier-naming)

namespace ume {

struct MetalBuffer {
    NS::SharedPtr<MTL::Buffer> buffer;
    size_t size = 0;
};

struct MetalPipeline {
    NS::SharedPtr<MTL::RenderPipelineState> state;
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

    [[nodiscard]] ShaderTarget shaderTarget() const override {
        return ShaderTarget::Msl;
    }

    void beginFrame() override;
    void draw(const DrawCommand &cmd) override;
    void postProcess(const PostProcessCommand &cmd) override;
    void endFrame() override;

    BufferHandle createBuffer(const BufferDescription &desc) override;
    void destroyBuffer(BufferHandle handle) override;

    PipelineHandle createPipeline(const PipelineDescription &desc) override;
    void destroyPipeline(PipelineHandle handle) override;

private:
    MetalSurface surface_;
    CA::MetalLayer *layer_ = nullptr;

    std::array<NS::SharedPtr<MTL::Texture>, 2> color_targets_;
    NS::SharedPtr<MTL::SamplerState> linear_sampler_;
    ResourcePool<MetalPipeline, PipelineHandle> pipelines_;

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

    NS::SharedPtr<MTL::Library>
    libraryFromMetallib(std::span<const std::byte> bytes);
    NS::SharedPtr<MTL::Library>
    libraryFromSource(std::span<const std::byte> bytes);

    NS::SharedPtr<MTL::RenderPipelineState> buildPipeline(MTL::Library *library,
                                                          const char *vert,
                                                          const char *frag,
                                                          bool with_depth);
};
} // namespace ume