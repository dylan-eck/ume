#pragma once

#include "ume/renderer/backend_types.hpp"

#include <Metal/Metal.hpp>

#include <utility>

namespace ume::metal {
inline MTL::LoadAction toMTLLoadAction(LoadAction action) {
    switch (action) {
    case LoadAction::DontCare:
        return MTL::LoadActionDontCare;
    case LoadAction::Load:
        return MTL::LoadActionLoad;
    case LoadAction::Clear:
        return MTL::LoadActionClear;
    }
    std::unreachable();
}

inline MTL::StoreAction toMTLStoreAction(StoreAction action) {
    switch (action) {
    case StoreAction::DontCare:
        return MTL::StoreActionDontCare;
    case StoreAction::Store:
        return MTL::StoreActionStore;
    }
    std::unreachable();
}

inline MTL::ClearColor toMTLClearColor(glm::vec4 color) {
    return {color[0], color[1], color[2], color[3]};
}

inline MTL::PixelFormat toMTLPixelFormat(Format format) {
    switch (format) {
    case Format::Undefined:
        return MTL::PixelFormatInvalid;
    case Format::BGRA8Unorm:
        return MTL::PixelFormatBGRA8Unorm;
    case Format::Depth32Float:
        return MTL::PixelFormatDepth32Float;
    }
    std::unreachable();
}

inline MTL::TextureUsage toMTLTextureUsage(TextureUsage usage) {
    MTL::TextureUsage mtl_usage = MTL::TextureUsageUnknown;
    if (hasFlag(usage, TextureUsage::RenderTarget)) {
        mtl_usage |= MTL::TextureUsageRenderTarget;
    }
    if (hasFlag(usage, TextureUsage::ShaderRead)) {
        mtl_usage |= MTL::TextureUsageShaderRead;
    }
    if (hasFlag(usage, TextureUsage::ShaderWrite)) {
        mtl_usage |= MTL::TextureUsageShaderWrite;
    }

    return mtl_usage;
}

inline MTL::SamplerMinMagFilter toMTLFilterType(Filter filter) {
    switch (filter) {
    case Filter::Nearest:
        return MTL::SamplerMinMagFilterNearest;
    case Filter::Linear:
        return MTL::SamplerMinMagFilterLinear;
    }
    std::unreachable();
}

inline MTL::SamplerAddressMode toMTLAddressMode(AddressMode mode) {
    switch (mode) {
    case AddressMode::ClampToEdge:
        return MTL::SamplerAddressModeClampToEdge;
    case AddressMode::MirrorClampToEdge:
        return MTL::SamplerAddressModeMirrorClampToEdge;
    case AddressMode::Repeat:
        return MTL::SamplerAddressModeRepeat;
    case AddressMode::MirrorRepeat:
        return MTL::SamplerAddressModeMirrorRepeat;
    case AddressMode::ClampToZero:
        return MTL::SamplerAddressModeClampToZero;
    case AddressMode::ClampToBorderColor:
        return MTL::SamplerAddressModeClampToBorderColor;
    }
    std::unreachable();
}

inline MTL::CullMode toMTLCullMode(CullMode mode) {
    switch (mode) {
    case CullMode::None:
        return MTL::CullModeNone;
    case CullMode::Back:
        return MTL::CullModeBack;
    case CullMode::Front:
        return MTL::CullModeFront;
    }
    std::unreachable();
}

inline MTL::Winding toMTLWinding(Winding winding) {
    switch (winding) {
    case Winding::CounterClockwise:
        return MTL::WindingCounterClockwise;
    case Winding::Clockwise:
        return MTL::WindingClockwise;
    }
    std::unreachable();
}

inline MTL::CompareFunction toMTLCompareFunction(CompareFunction function) {
    switch (function) {
    case CompareFunction::Never:
        return MTL::CompareFunctionNever;
    case CompareFunction::Less:
        return MTL::CompareFunctionLess;
    case CompareFunction::Equal:
        return MTL::CompareFunctionEqual;
    case CompareFunction::LessEqual:
        return MTL::CompareFunctionLessEqual;
    case CompareFunction::NotEqual:
        return MTL::CompareFunctionNotEqual;
    case CompareFunction::Greater:
        return MTL::CompareFunctionGreater;
    case CompareFunction::GreaterEqual:
        return MTL::CompareFunctionGreaterEqual;
    case CompareFunction::Always:
        return MTL::CompareFunctionAlways;
    }
    std::unreachable();
}
} // namespace ume::metal
