#pragma once

#include "ume/core/resource_handle.hpp"

#include <memory>
#include <span>

namespace ume {

struct BufferDescription {
    size_t size;
    const void *initial_data;
};

enum class IndexType : uint8_t { UInt16, UInt32 };

enum class ShaderTarget : uint8_t { Msl, SpirV };

struct PipelineDescription {
    std::span<const std::byte> shader;
    const char *vertex_entry = "vertMain";
    const char *fragment_entry = "fragMain";
};

struct DrawCommand {
    BufferHandle vertex_buffer;
    BufferHandle index_buffer;
    uint32_t index_count = 0;
    IndexType index_type;
    std::span<const std::byte> push_constants;
};

struct PostProcessPass {
    PipelineHandle pipeline;
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

    virtual void beginFrame() = 0;
    virtual void draw(const DrawCommand &cmd) = 0;
    virtual void postProcess(const PostProcessCommand &cmd) = 0;
    virtual void endFrame() = 0;

    virtual BufferHandle createBuffer(const BufferDescription &desc) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;

    virtual PipelineHandle createPipeline(const PipelineDescription &desc) = 0;
    virtual void destroyPipeline(PipelineHandle handle) = 0;
};

class Window;
std::unique_ptr<RendererBackend> createRendererBackend(const Window &window);
} // namespace ume