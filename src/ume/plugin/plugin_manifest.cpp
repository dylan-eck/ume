#include "plugin_manifest.hpp"
#include "ume/core/logger.hpp"

#include <glaze/toml.hpp>

namespace ume {
std::optional<PluginManifest> loadManifest(const std::filesystem::path &dir) {
    const std::filesystem::path path = dir / "plugin.toml";
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }
    PluginManifest manifest;
    glz::error_ctx err =
        glz::read_file_toml(manifest, path.string(), std::string{});
    if (err) {
        UME_LOG_ERROR(Plugin, "failed to parse {}: {}", path.string(),
                      glz::format_error(err, "plugin.toml"));
        return std::nullopt;
    }
    return manifest;
}
} // namespace ume