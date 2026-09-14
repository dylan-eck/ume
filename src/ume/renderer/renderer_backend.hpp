#pragma once

#include "ume/core/resource_handle.hpp"

#include <memory>
#include <span>
#include <array>

namespace ume {

enum class BufferUsage : uint8_t { CpuToGpu, GpuOnly };

struct BufferDescription {
    size_t size;
    const void *initial_data = nullptr;
    BufferUsage usage = BufferUsage::CpuToGpu;
};

struct TextureDescription {
    uint32_t width;
    uint32_t height;
    // TODO: add format
};

enum class IndexType : uint8_t { UInt16, UInt32 };

enum class ShaderTarget : uint8_t { Msl, SpirV };

// TODO: maybe don't have default entry names
struct GraphicsPipelineDescription {
    std::span<const std::byte> shader;
    const char *vertex_entry = "vertMain";
    const char *fragment_entry = "fragMain";
};

struct ComputePipelineDescription {
    std::span<const std::byte> shader;
    std::array<uint32_t, 3> workgroup_size = {1, 1, 1};
    const char *entry = "main";
};

struct BufferBinding {
    uint32_t slot = 0;
    BufferHandle buffer;
    uint32_t offset = 0;
};

struct TextureBinding {
    uint32_t slot = 0;
    TextureHandle texture;
};

struct SamplerBinding {
    uint32_t slot = 0;
    SamplerHandle sampler;
};

struct ResourceBindings {
    std::span<const BufferBinding> buffers;
    std::span<const TextureBinding> textures;
    std::span<const SamplerBinding> samplers;
    std::span<const std::byte> params;
    uint32_t params_slot = 0;
};

struct DrawCommand {
    BufferHandle vertex_buffer;
    BufferHandle index_buffer;
    uint32_t index_count = 0;
    IndexType index_type;
    std::span<const std::byte> push_constants;
};

struct DispatchCommand {
    ComputePipelineHandle pipeline;
    std::array<uint32_t, 3> work_size = {1, 1, 1};
    ResourceBindings bindings;
};

struct PostProcessPass {
    GraphicsPipelineHandle pipeline;
    std::span<const std::byte> params;
};

struct PostProcessCommand {
    std::span<const std::byte> frame_uniforms;
    std::span<const PostProcessPass> passes;
};

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