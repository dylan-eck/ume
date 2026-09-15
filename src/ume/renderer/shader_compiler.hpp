#pragma once

#include "ume/renderer/renderer_backend.hpp"

#include <slang-com-ptr.h>
#include <slang.h>

#include <filesystem>
#include <optional>
#include <vector>

namespace ume {

struct CompiledShader {
    std::vector<std::byte> code;
    std::vector<EntryPoint> entry_points;
    // module scope parameters, shared by every entry point
    std::vector<ShaderBinding> bindings;
};

class ShaderCompiler {
public:
    ShaderCompiler(ShaderTarget target, std::filesystem::path include_dir);

    std::optional<CompiledShader> compile(const std::filesystem::path &path);

private:
    Slang::ComPtr<slang::IGlobalSession> global_;
    ShaderTarget target_;
    std::filesystem::path include_dir_;

    Slang::ComPtr<slang::ISession> createSession();
};
} // namespace ume