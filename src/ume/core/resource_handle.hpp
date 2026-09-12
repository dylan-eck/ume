#pragma once

#include <cstdint>

namespace ume {
template <typename Tag> struct ResourceHandle {
    uint32_t id = 0;
    explicit operator bool() const { return id != 0; }
    friend bool operator==(ResourceHandle, ResourceHandle) = default;
};

struct BufferTag;
struct MeshTag;
struct ObjectTag;
struct GraphicsPipelineTag;
struct ComputePipelineTag;
struct PostEffectTag;
struct TextureTag;
struct SamplerTag;

using BufferHandle = ResourceHandle<BufferTag>;
using MeshHandle = ResourceHandle<MeshTag>;
using ObjectHandle = ResourceHandle<ObjectTag>;
using GraphicsPipelineHandle = ResourceHandle<GraphicsPipelineTag>;
using ComputePipelineHandle = ResourceHandle<ComputePipelineTag>;
using PostEffectHandle = ResourceHandle<PostEffectTag>;
using TextureHandle = ResourceHandle<TextureTag>;
using SamplerHandle = ResourceHandle<SamplerTag>;
} // namespace ume