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
ShaderCompiler::compileCompute(const std::filesystem::path &path,
                               const char *entrypoint_name) {
    // TODO: reading file into string could be a function
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

    Slang::ComPtr<slang::IEntryPoint> entry;
    module->findEntryPointByName(entrypoint_name, entry.writeRef());
    if (entry == nullptr) {
        UME_LOG_ERROR(Renderer, "'{}' must define {}", path.string(),
                      entrypoint_name);
        return std::nullopt;
    }

    // TODO: don't repeat yourself
    std::array<slang::IComponentType *, 2> parts = {module, entry};
    Slang::ComPtr<slang::IComponentType> composed;
    Slang::ComPtr<slang::IComponentType> linked;
    if (SLANG_FAILED(session->createCompositeComponentType(
            parts.data(), 2, composed.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }
    if (SLANG_FAILED(composed->link(linked.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }

    Slang::ComPtr<slang::IBlob> code;
    if (SLANG_FAILED(
            linked->getTargetCode(0, code.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }

    CompiledShader out;
    const auto *bytes =
        static_cast<const std::byte *>(code->getBufferPointer());
    out.code.assign(bytes, bytes + code->getBufferSize());

    slang::ProgramLayout *layout = linked->getLayout(0);
    slang::EntryPointReflection *ep = layout->getEntryPointByIndex(0);
    std::array<SlangUInt, 3> sizes = {1, 1, 1};
    ep->getComputeThreadGroupSize(3, sizes.data());
    out.workgroup_size = {
        static_cast<uint32_t>(sizes[0]),
        static_cast<uint32_t>(sizes[1]),
        static_cast<uint32_t>(sizes[2]),
    };

    for (unsigned i = 0; i < layout->getParameterCount(); i++) {
        slang::VariableLayoutReflection *param = layout->getParameterByIndex(i);
        if (param->getName() != nullptr &&
            std::string_view(param->getName()) == "params") {
            out.params_size = static_cast<uint32_t>(
                param->getTypeLayout()->getElementTypeLayout()->getSize());
        }
    }

    std::cout << reinterpret_cast<const char *>(out.code.data()) << "\n";

    return out;
}

std::optional<CompiledShader>
ShaderCompiler::compilePostEffect(const std::filesystem::path &path) {
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

    Slang::ComPtr<slang::IEntryPoint> vert;
    Slang::ComPtr<slang::IEntryPoint> frag;
    module->findEntryPointByName("vertMain", vert.writeRef());
    module->findEntryPointByName("fragMain", frag.writeRef());
    if (vert == nullptr || frag == nullptr) {
        UME_LOG_ERROR(Renderer, "'{}' must define vertMain and fragMain",
                      path.string());
        return std::nullopt;
    }

    std::array<slang::IComponentType *, 3> parts = {module, vert, frag};
    Slang::ComPtr<slang::IComponentType> composed;
    Slang::ComPtr<slang::IComponentType> linked;
    if (SLANG_FAILED(session->createCompositeComponentType(
            parts.data(), 3, composed.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }
    if (SLANG_FAILED(composed->link(linked.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }

    Slang::ComPtr<slang::IBlob> code;
    if (SLANG_FAILED(
            linked->getTargetCode(0, code.writeRef(), diag.writeRef()))) {
        logDiagnostics(diag);
        return std::nullopt;
    }

    CompiledShader out;
    const auto *bytes =
        static_cast<const std::byte *>(code->getBufferPointer());
    out.code.assign(bytes, bytes + code->getBufferSize());

    slang::ProgramLayout *layout = linked->getLayout(0);
    for (unsigned i = 0; i < layout->getParameterCount(); i++) {
        slang::VariableLayoutReflection *param = layout->getParameterByIndex(i);
        if (param->getName() != nullptr &&
            std::string_view(param->getName()) == "params") {
            out.params_size = static_cast<uint32_t>(
                param->getTypeLayout()->getElementTypeLayout()->getSize());
        }
    }

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
