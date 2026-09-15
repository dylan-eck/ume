#include "shader_compiler.hpp"
#include "ume/core/error.hpp"
#include "ume/core/logger.hpp"

#include <fstream>
#include <string>
#include <string_view>
#include <array>
#include <iostream>

namespace ume {
namespace {
void logDiagnostics(slang::IBlob *diag) {
    if (diag == nullptr || diag->getBufferSize() <= 0) return;

    UME_LOG_ERROR(
        Renderer, "slang error: {}",
        std::string_view(static_cast<const char *>(diag->getBufferPointer()),
                         diag->getBufferSize()));
}

constexpr bool kLogReflection = true;

const char *nameOr(const char *name, const char *fallback) {
    return name != nullptr ? name : fallback;
}

std::optional<ShaderStage> toShaderStage(SlangStage stage) {
    switch (stage) {
    case SLANG_STAGE_VERTEX:
        return ShaderStage::Vertex;
    case SLANG_STAGE_FRAGMENT:
        return ShaderStage::Fragment;
    case SLANG_STAGE_COMPUTE:
        return ShaderStage::Compute;
    default:
        return std::nullopt;
    }
}

std::optional<BindingType> toBindingType(slang::TypeLayoutReflection *type) {
    if (type == nullptr) {
        return std::nullopt;
    }

    switch (type->getKind()) {
    case slang::TypeReflection::Kind::SamplerState:
        return BindingType::Sampler;

    case slang::TypeReflection::Kind::ConstantBuffer:
    case slang::TypeReflection::Kind::ParameterBlock:
    case slang::TypeReflection::Kind::ShaderStorageBuffer:
        return BindingType::Buffer;

    case slang::TypeReflection::Kind::Resource:
        switch (type->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK) {
        case SLANG_STRUCTURED_BUFFER:
        case SLANG_BYTE_ADDRESS_BUFFER:
            return BindingType::Buffer;
        case SLANG_TEXTURE_1D:
        case SLANG_TEXTURE_2D:
        case SLANG_TEXTURE_3D:
        case SLANG_TEXTURE_CUBE:
            return BindingType::Texture;
        default:
            return std::nullopt;
        }

    default:
        return std::nullopt;
    }
}
} // namespace

ShaderCompiler::ShaderCompiler(ShaderTarget target,
                               std::filesystem::path include_dir)
    : target_(target), include_dir_(std::move(include_dir)) {
    if (SLANG_FAILED(slang::createGlobalSession(global_.writeRef()))) {
        throw Error(logger::Category::Renderer,
                    "failed to create slang global session");
    }
}

std::optional<CompiledShader>
ShaderCompiler::compile(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        UME_LOG_ERROR(Renderer, "cannot read shader '{}'", path.string());
        return std::nullopt;
    }
    const std::string source((std::istreambuf_iterator<char>(file)), {});

    Slang::ComPtr<slang::ISession> session = createSession();

    if (session == nullptr) return std::nullopt;

    Slang::ComPtr<slang::IBlob> diag;
    const std::string module_name = path.stem().string();
    slang::IModule *module = session->loadModuleFromSourceString(
        module_name.c_str(), path.string().c_str(), source.c_str(),
        diag.writeRef());
    logDiagnostics(diag);

    if (module == nullptr) return std::nullopt;

    SlangInt32 defined_entry_count = module->getDefinedEntryPointCount();
    std::vector<Slang::ComPtr<slang::IEntryPoint>> defined_entry_points(
        defined_entry_count);
    std::vector<slang::IComponentType *> parts;
    parts.reserve(defined_entry_count + 1);
    parts.push_back(module);

    for (SlangInt32 i = 0; i < defined_entry_count; i++) {
        if (SLANG_FAILED(module->getDefinedEntryPoint(
                i, defined_entry_points[i].writeRef()))) {
            return std::nullopt;
        }
        parts.push_back(defined_entry_points[i]);
    }

    Slang::ComPtr<slang::IComponentType> composed;
    Slang::ComPtr<slang::IComponentType> linked;
    if (SLANG_FAILED(session->createCompositeComponentType(
            parts.data(), static_cast<SlangInt>(parts.size()),
            composed.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }

    if (SLANG_FAILED(composed->link(linked.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }

    slang::ProgramLayout *layout = linked->getLayout(0);

    CompiledShader out;

    const unsigned param_count = layout->getParameterCount();
    out.bindings.reserve(param_count);

    for (unsigned i = 0; i < param_count; i++) {
        slang::VariableLayoutReflection *param = layout->getParameterByIndex(i);
        const char *name = nameOr(param->getName(), "<unnamed>");

        if (kLogReflection) {
            UME_LOG_INFO(
                Renderer, "  '{}' parameter '{}' category {} binding {}",
                path.string(), name, static_cast<int>(param->getCategory()),
                param->getBindingIndex());
        }

        std::optional<BindingType> type = toBindingType(param->getTypeLayout());
        if (!type) {
            UME_LOG_WARN(Renderer,
                         "'{}': ignoring parameter '{}', unsupported type",
                         path.string(), name);
            continue;
        }

        // uniform blocks are declared at module scope and shared by every
        // entry point, so their size belongs here rather than on an entry point
        uint32_t size = 0;
        const slang::TypeReflection::Kind kind =
            param->getTypeLayout()->getKind();
        if (kind == slang::TypeReflection::Kind::ConstantBuffer ||
            kind == slang::TypeReflection::Kind::ParameterBlock) {
            size = static_cast<uint32_t>(
                param->getTypeLayout()->getElementTypeLayout()->getSize());
        }

        out.bindings.push_back({
            .name = name,
            .type = *type,
            .slot = param->getBindingIndex(),
            .size = size,
        });
    }

    const SlangUInt entry_point_count = layout->getEntryPointCount();
    out.entry_points.reserve(entry_point_count);

    for (SlangUInt i = 0; i < entry_point_count; i++) {
        slang::EntryPointReflection *ep = layout->getEntryPointByIndex(i);
        const char *name = nameOr(ep->getName(), "<unnamed>");

        std::optional<ShaderStage> stage = toShaderStage(ep->getStage());
        if (!stage) {
            UME_LOG_WARN(Renderer,
                         "'{}': ignoring entry point '{}', unsupported stage",
                         path.string(), name);
            continue;
        }

        EntryPoint entry_point;
        entry_point.name = name;
        entry_point.stage = *stage;

        if (*stage == ShaderStage::Compute) {
            std::array<SlangUInt, 3> workgroup_size = {1, 1, 1};
            ep->getComputeThreadGroupSize(workgroup_size.size(),
                                          workgroup_size.data());
            entry_point.workgroup_size = {
                static_cast<uint32_t>(workgroup_size[0]),
                static_cast<uint32_t>(workgroup_size[1]),
                static_cast<uint32_t>(workgroup_size[2]),
            };
        }

        if (kLogReflection) {
            const unsigned entry_param_count = ep->getParameterCount();
            for (unsigned j = 0; j < entry_param_count; j++) {
                slang::VariableLayoutReflection *param =
                    ep->getParameterByIndex(j);
                UME_LOG_INFO(Renderer,
                             "  '{}' entry point '{}' parameter '{}' "
                             "category {} binding {}",
                             path.string(), name,
                             nameOr(param->getName(), "<unnamed>"),
                             static_cast<int>(param->getCategory()),
                             param->getBindingIndex());
            }
        }

        out.entry_points.push_back(std::move(entry_point));
    }

    if (out.entry_points.empty()) {
        UME_LOG_ERROR(Renderer,
                      "'{}' defines no usable entry points; every entry point "
                      "needs a [shader(\"...\")] attribute",
                      path.string());
        return std::nullopt;
    }

    Slang::ComPtr<slang::IBlob> code;
    if (SLANG_FAILED(
            linked->getTargetCode(0, code.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }

    const auto *bytes =
        static_cast<const std::byte *>(code->getBufferPointer());
    out.code.assign(bytes, bytes + code->getBufferSize());

    return out;
}

Slang::ComPtr<slang::ISession> ShaderCompiler::createSession() {
    slang::TargetDesc target_desc{};
    if (target_ == ShaderTarget::Msl) {
        target_desc.format = SLANG_METAL;
    } else {
        target_desc.format = SLANG_SPIRV;
        target_desc.profile = global_->findProfile("spirv_1_4");
    }

    std::array<const char *, 1> search_paths = {include_dir_.c_str()};

    slang::SessionDesc desc{};
    desc.targets = &target_desc;
    desc.targetCount = 1;
    desc.searchPaths = search_paths.data();
    desc.searchPathCount = 1;
    desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(global_->createSession(desc, session.writeRef()))) {
        UME_LOG_ERROR(Renderer, "failed to create slang session");
    }
    return session;
}

} // namespace ume
