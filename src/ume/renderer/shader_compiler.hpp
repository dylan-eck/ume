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
    uint32_t params_size = 0;
};

class ShaderCompiler {
public:
    ShaderCompiler(ShaderTarget target, std::filesystem::path include_dir);

    std::optional<CompiledShader>
    compilePostEffect(const std::filesystem::path &path);

private:
    Slang::ComPtr<slang::IGlobalSession> global_;
    ShaderTarget target_;
    std::filesystem::path include_dir_;

    Slang::ComPtr<slang::ISession> createSession();
};
} // namespace ume