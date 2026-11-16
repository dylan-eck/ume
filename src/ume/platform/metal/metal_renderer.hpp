#pragma once

#include "ume/core/resource_handle.hpp"
#include "ume/renderer/backend_types.hpp"
#include "ume/renderer/renderer_backend.hpp"
#include "ume/core/resource_pool.hpp"
#include "ume/platform/window.hpp"

#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <Metal/Metal.hpp>

#include <array>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>
#include <span>

namespace ume {

struct MetalBuffer {
    NS::SharedPtr<MTL::Buffer> buffer;
    size_t size = 0;
};

struct MetalShader {
    NS::SharedPtr<MTL::Library> library;
    std::vector<EntryPoint> entry_points;
};

struct MetalGraphicsPipeline {
    GraphicsPipelineDescription description;
    NS::SharedPtr<MTL::RenderPipelineState> state;
    NS::SharedPtr<MTL::DepthStencilState> depth_state;
};

struct MetalComputePipeline {
    NS::SharedPtr<MTL::ComputePipelineState> state;
    std::array<uint32_t, 3> workgroup_size = {1, 1, 1};
};

struct MetalTexture {
    TextureDescription description;
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

    [[nodiscard]] TextureHandle backbuffer() const override;

    void beginFrame() override;
    void beginRenderPass(const RenderPassDescription &desc) override;
    void endRenderPass() override;
    void draw(const DrawCommand &cmd) override;
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
    void copyTexture(TextureHandle src, TextureHandle dst) override;

    SamplerHandle createSampler(const SamplerDescription &desc) override;
    void destroySampler(SamplerHandle handle) override;

    ShaderHandle createShader(const ShaderDescription &desc) override;
    void destroyShader(ShaderHandle handle) override;

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

    TextureHandle backbuffer_;

    ResourcePool<MetalShader, ShaderHandle> shaders_;

    ResourcePool<MetalGraphicsPipeline, GraphicsPipelineHandle>
        graphics_pipelines_;
    GraphicsPipelineHandle curr_graphics_pipeline_{};

    ResourcePool<MetalComputePipeline, ComputePipelineHandle>
        compute_pipelines_;

    NS::SharedPtr<MTL::Device> device_ = nullptr;
    NS::SharedPtr<MTL::CommandQueue> command_queue_ = nullptr;

    NS::SharedPtr<NS::AutoreleasePool> frame_pool_ = nullptr;
    CA::MetalDrawable *drawable_ = nullptr;
    MTL::CommandBuffer *command_buffer_ = nullptr;
    MTL::RenderCommandEncoder *graphics_encoder_ = nullptr;
    std::string last_error_;

    bool curr_frame_failed_ = false;
    std::optional<RenderPassDescription> curr_pass_desc_;
    std::array<Format, kMaxColorAttachments> curr_pass_color_formats_{};
    Format curr_pass_depth_format_ = Format::Undefined;

    MTL::ComputeCommandEncoder *compute_encoder_ = nullptr;

    ResourcePool<MetalBuffer, BufferHandle> buffers_;

    NS::SharedPtr<MTL::Library>
    libraryFromSource(std::span<const std::byte> bytes);

    NS::SharedPtr<MTL::RenderPipelineState>
    buildGraphicsPipeline(const GraphicsPipelineDescription &desc,
                          MTL::Library *library);

    NS::SharedPtr<MTL::ComputePipelineState>
    buildComputePipeline(MTL::Library *library, const char *entry);

    MTL::Buffer *getBuffer(BufferHandle handle);
    MTL::Texture *getTexture(TextureHandle handle);
    MTL::SamplerState *getSampler(SamplerHandle handle);

    bool acquireBackbuffer();
    void failFrame(std::string message);

    template <typename Binder>
    bool bindResources(const Binder &binder, const ResourceBindings &bindings);

    std::expected<void, std::string>
    validateRenderPass(const RenderPassDescription &desc);
    [[nodiscard]] bool isCurrentPassAttachment(TextureHandle handle) const;
};
} // namespace ume