#pragma once

#include "ume/core/resource_handle.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace ume {
enum class BufferUsage : uint8_t { CpuToGpu, GpuOnly };

enum class TextureUsage : uint8_t {
    None = 0,
    RenderTarget = 1 << 0,
    ShaderRead = 1 << 1,
    ShaderWrite = 1 << 2,
};

constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(std::to_underlying(a) |
                                     std::to_underlying(b));
}

constexpr TextureUsage operator&(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(std::to_underlying(a) &
                                     std::to_underlying(b));
}

constexpr bool hasFlag(TextureUsage set, TextureUsage flag) {
    return (set & flag) == flag;
}

// this limit is set by metal
constexpr size_t kMaxColorAttachments = 8;

enum class IndexType : uint8_t { UInt16, UInt32 };

enum class ShaderTarget : uint8_t { Msl };

enum class ShaderStage : uint8_t { Vertex, Fragment, Compute };

enum class BindingType : uint8_t { Buffer, Texture, Sampler };
constexpr std::string_view toString(BindingType type) {
    switch (type) {
    case BindingType::Buffer:
        return "buffer";
    case BindingType::Texture:
        return "texture";
    case BindingType::Sampler:
        return "sampler";
    }
    std::unreachable();
}

enum class Format : uint8_t { Undefined, BGRA8Unorm, Depth32Float };

enum class Filter : uint8_t { Nearest, Linear };

enum class LoadAction : uint8_t { DontCare, Load, Clear };

enum class StoreAction : uint8_t { DontCare, Store };

enum class AddressMode : uint8_t {
    ClampToEdge,
    MirrorClampToEdge,
    Repeat,
    MirrorRepeat,
    ClampToZero,
    ClampToBorderColor,
};

enum class CullMode : uint8_t { None, Back, Front };

enum class Winding : uint8_t { CounterClockwise, Clockwise };

enum class CompareFunction : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    NotEqual,
    Greater,
    GreaterEqual,
    Always,
};

struct BufferDescription {
    size_t size;
    const void *initial_data = nullptr;
    BufferUsage usage = BufferUsage::CpuToGpu;
};

struct TextureDescription {
    uint32_t width;
    uint32_t height;
    Format format;
    TextureUsage usage;
};

struct SamplerDescription {
    Filter min_filter;
    Filter mag_filter;
    AddressMode address_mode;
};

struct ShaderBinding {
    std::string name;
    BindingType type = BindingType::Buffer;
    uint32_t slot = 0;
    // byte size of the block, for constant buffers; zero for everything else
    uint32_t size = 0;
};

struct EntryPoint {
    std::string name;
    ShaderStage stage = ShaderStage::Compute;
    std::array<uint32_t, 3> workgroup_size{1, 1, 1};
};

struct ShaderDescription {
    std::span<const std::byte> code;
    std::span<const EntryPoint> entry_points;
};

struct RasterState {
    CullMode cull_mode = CullMode::None;
    Winding front_face_winding = Winding::CounterClockwise;
};

struct DepthState {
    CompareFunction compare = CompareFunction::Always;
    bool write = false;
};

struct GraphicsPipelineDescription {
    ShaderHandle shader;
    std::string vertex_entry;
    std::string fragment_entry;
    std::array<Format, kMaxColorAttachments> color_formats;
    Format depth_format = Format::Undefined;
    DepthState depth_state;
    RasterState raster_state;
};

struct ColorAttachment {
    TextureHandle texture;
    LoadAction load_action = LoadAction::Clear;
    StoreAction store_action = StoreAction::Store;
    glm::vec4 clear_value{0.0f};
};

struct DepthAttachment {
    TextureHandle texture;
    LoadAction load_action = LoadAction::Clear;
    StoreAction store_action = StoreAction::Store;
    float clear_value = 0.0f;
};

struct RenderPassDescription {
    std::array<ColorAttachment, kMaxColorAttachments> color_attachments;
    std::optional<DepthAttachment> depth_attachment{};
};

struct ComputePipelineDescription {
    ShaderHandle shader;
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

struct InlineDataBinding {
    uint32_t slot;
    std::span<const std::byte> data;
};

struct ResourceBindings {
    std::span<const BufferBinding> buffers;
    std::span<const TextureBinding> textures;
    std::span<const SamplerBinding> samplers;
    std::span<const InlineDataBinding> inline_data;
};

struct DrawCommand {
    GraphicsPipelineHandle pipeline;
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;
    BufferHandle index_buffer{};
    IndexType index_type = IndexType::UInt32;
    ResourceBindings vertex_bindings{};
    ResourceBindings fragment_bindings{};
};

struct DispatchCommand {
    ComputePipelineHandle pipeline;
    std::array<uint32_t, 3> work_size = {1, 1, 1};
    ResourceBindings bindings{};
};
} // namespace ume
