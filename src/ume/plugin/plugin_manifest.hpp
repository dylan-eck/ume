#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ume {
struct PostEffectManifest {
    std::string name;
    std::string shader; // relative to the manifest
};

struct PluginManifest {
    std::string library; // optional
    std::vector<PostEffectManifest> post_effects;
};

std::optional<PluginManifest> loadManifest(const std::filesystem::path &dir);
} // namespace ume