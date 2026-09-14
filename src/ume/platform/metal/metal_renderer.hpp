#pragma once

#include "ume/renderer/renderer_backend.hpp"
#include "ume/core/resource_pool.hpp"
#include "ume/platform/window.hpp"

#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <Metal/Metal.hpp>

#include <array>

namespace ume {

struct MetalBuffer {
    NS::SharedPtr<MTL::Buffer> buffer;
    size_t size = 0;
};

struct MetalGraphicsPipeline {
    NS::SharedPtr<MTL::RenderPipelineState> state;
};

struct MetalComputePipeline {
    NS::SharedPtr<MTL::ComputePipelineState> state;
    std::array<uint32_t, 3> workgroup_size = {1, 1, 1};
};

struct MetalTexture {
    NS::SharedPtr<MTL::Texture> texture;
};

struct MetalSampler {
    NS::SharedPtr<MTL::SamplerState> state;
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

    void resize(uint32_t width, uint32_t height) override;

    void beginFrame() override;
    void beginScenePass() override;
    void draw(const DrawCommand &cmd) override;
    void postProcess(const PostProcessCommand &cmd) override;
    void endFrame() override;

    void beginComputePass() override;
    void dispatch(const DispatchCommand &cmd) override;
    void endComputePass() override;

    BufferHandle createBuffer(const BufferDescription &desc) override;
    void destroyBuffer(BufferHandle handle) override;
    void readBuffer(BufferHandle handle, size_t offset,
                    std::span<std::byte> out) override;

    TextureHandle createTexture(const TextureDescription &desc) override;
    void destroyTexture(TextureHandle handle) override;

    GraphicsPipelineHandle
    createGraphicsPipeline(const GraphicsPipelineDescription &desc) override;
    void destroyGraphicsPipeline(GraphicsPipelineHandle handle) override;
    ComputePipelineHandle
    createComputePipeline(const ComputePipelineDescription &desc) override;
    void destroyComputePipeline(ComputePipelineHandle handle) override;

private:
    uint32_t width_;
    uint32_t height_;

    MetalSurface surface_;
    CA::MetalLayer *layer_ = nullptr;

    ResourcePool<MetalSampler, SamplerHandle> samplers_;
    ResourcePool<MetalTexture, TextureHandle> textures_;

    std::array<TextureHandle, 2> color_targets_;
    SamplerHandle linear_sampler_;

    ResourcePool<MetalGraphicsPipeline, GraphicsPipelineHandle>
        graphics_pipelines_;
    ResourcePool<MetalComputePipeline, ComputePipelineHandle>
        compute_pipelines_;

    NS::SharedPtr<MTL::Device> device_ = nullptr;
    NS::SharedPtr<MTL::CommandQueue> command_queue_ = nullptr;
    NS::SharedPtr<MTL::RenderPipelineState> pipeline_state_ = nullptr;
    TextureHandle depth_texture_;
    NS::SharedPtr<MTL::DepthStencilState> depth_state_;

    NS::SharedPtr<NS::AutoreleasePool> frame_pool_ = nullptr;
    CA::MetalDrawable *drawable_ = nullptr;
    MTL::CommandBuffer *command_buffer_ = nullptr;
    MTL::RenderCommandEncoder *graphics_encoder_ = nullptr;

    NS::SharedPtr<NS::AutoreleasePool> compute_pool_ = nullptr;
    MTL::ComputeCommandEncoder *compute_encoder_ = nullptr;

    ResourcePool<MetalBuffer, BufferHandle> buffers_;

    NS::SharedPtr<MTL::Library>
    libraryFromMetallib(std::span<const std::byte> bytes);
    NS::SharedPtr<MTL::Library>
    libraryFromSource(std::span<const std::byte> bytes);

    NS::SharedPtr<MTL::RenderPipelineState>
    buildGraphicsPipeline(MTL::Library *library, const char *vert,
                          const char *frag, bool with_depth);

    NS::SharedPtr<MTL::ComputePipelineState>
    buildComputePipeline(MTL::Library *library, const char *entry);

    void createRenderTargets(uint32_t width, uint32_t height);

    MTL::Buffer *getBuffer(BufferHandle handle);
    MTL::Texture *getTexture(TextureHandle handle);
    MTL::SamplerState *getSampler(SamplerHandle handle);
};
} // namespace ume