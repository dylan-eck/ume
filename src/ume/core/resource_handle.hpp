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
struct PipelineTag;
struct PostEffectTag;

using BufferHandle = ResourceHandle<BufferTag>;
using MeshHandle = ResourceHandle<MeshTag>;
using ObjectHandle = ResourceHandle<ObjectTag>;
using PipelineHandle = ResourceHandle<PipelineTag>;
using PostEffectHandle = ResourceHandle<PostEffectTag>;
} // namespace ume