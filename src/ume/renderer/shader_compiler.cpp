#include "shader_compiler.hpp"
#include "ume/core/error.hpp"
#include "ume/core/logger.hpp"

#include <fstream>
#include <string>
#include <string_view>
#include <array>

namespace ume {
namespace {
void logDiagnostics(slang::IBlob *diag) {
    if (diag == nullptr || diag->getBufferSize() <= 0) {
        return;
    }

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

Slang::ComPtr<slang::ISession> ShaderCompiler::createSession() {
    slang::TargetDesc target_desc{};
    if (target_ == ShaderTarget::Msl) {
        target_desc.format = SLANG_METAL;
    } else {
        target_desc.format = SLANG_SPIRV;
        target_desc.profile = global_->findProfile("spirv_1_4");
    }

    const std::string include = include_dir_.string();
    std::array<const char *, 1> search_paths = {include.c_str()};

    slang::SessionDesc desc{};
    desc.targets = &target_desc;
    desc.targetCount = 1;
    desc.searchPaths = search_paths.data();
    desc.searchPathCount = 1;

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(global_->createSession(desc, session.writeRef()))) {
        UME_LOG_ERROR(Renderer, "failed to create slang session");
    }
    return session;
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
    if (session == nullptr) {
        return std::nullopt;
    }

    Slang::ComPtr<slang::IBlob> diag;
    const std::string module_name = path.stem().string();
    slang::IModule *module = session->loadModuleFromSourceString(
        module_name.c_str(), path.string().c_str(), source.c_str(),
        diag.writeRef());
    logDiagnostics(diag);
    if (module == nullptr) {
        return std::nullopt;
    }

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
} // namespace ume
