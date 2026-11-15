#pragma once

#include "ume/core/resource_handle.hpp"
#include "ume/renderer/backend_types.hpp"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <span>

namespace ume {

class RendererBackend {
public:
    RendererBackend() = default;
    virtual ~RendererBackend() = default;

    RendererBackend(const RendererBackend &) = delete;
    RendererBackend &operator=(const RendererBackend &) = delete;

    RendererBackend(RendererBackend &&) = delete;
    RendererBackend &operator=(RendererBackend &&) = delete;

    [[nodiscard]] virtual ShaderTarget shaderTarget() const = 0;

    // resize() can only be called before beginFrame() or after endFrame()
    virtual void resize(uint32_t width, uint32_t height) = 0;

    virtual void beginFrame() = 0;
    virtual void beginRenderPass(const RenderPassDescription &desc) = 0;
    virtual void endRenderPass() = 0;
    virtual void beginScenePass() = 0;
    virtual void draw(const DrawCommand &cmd) = 0;
    virtual void postProcess(const PostProcessCommand &cmd) = 0;
    virtual void endFrame() = 0;

    virtual void beginComputePass() = 0;
    virtual void dispatch(const DispatchCommand &cmd) = 0;
    virtual void endComputePass() = 0;

    virtual BufferHandle createBuffer(const BufferDescription &desc) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;
    virtual void readBuffer(BufferHandle handle, size_t offset,
                            std::span<std::byte> out) = 0;

    virtual TextureHandle createTexture(const TextureDescription &desc) = 0;
    virtual void destroyTexture(TextureHandle handle) = 0;

    virtual SamplerHandle createSampler(const SamplerDescription &desc) = 0;
    virtual void destroySampler(SamplerHandle handle) = 0;

    virtual ShaderHandle createShader(const ShaderDescription &desc) = 0;
    virtual void destroyShader(ShaderHandle handle) = 0;

    virtual GraphicsPipelineHandle
    createGraphicsPipeline(const GraphicsPipelineDescription &desc) = 0;
    virtual void destroyGraphicsPipeline(GraphicsPipelineHandle handle) = 0;
    virtual ComputePipelineHandle
    createComputePipeline(const ComputePipelineDescription &desc) = 0;
    virtual void destroyComputePipeline(ComputePipelineHandle handle) = 0;
};

class Window;
std::unique_ptr<RendererBackend> createRendererBackend(const Window &window);
} // namespace ume