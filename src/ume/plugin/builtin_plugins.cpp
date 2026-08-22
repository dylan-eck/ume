#include "ume/plugin/plugin_host.hpp"
#include "ume/core/logger.hpp"

#include <filesystem>

#ifndef UME_PLUGIN_SUFFIX
#define UME_PLUGIN_SUFFIX ".so"
#endif

inline constexpr std::string_view kPluginSuffix = UME_PLUGIN_SUFFIX;

#ifdef UME_PLUGIN_STATIC
extern "C" UME_PLUGIN_BOOL
    UME_PLUGIN_ENTRY(procPlanet)(const UmePluginApi *api,
                                 UmePluginDescription *description);
#endif

namespace ume {
std::filesystem::path
findPluginLibrary(const std::filesystem::path &plugin_dir) {
    std::error_code ec;

    for (const auto &entry :
         std::filesystem::directory_iterator(plugin_dir, ec)) {
        if (!entry.is_regular_file(ec)) {
            continue;
        }

        if (entry.path().extension() == kPluginSuffix) {
            return entry.path();
        }
    }
    return {};
}

void loadPluginsFrom(PluginHost &host,
                     const std::filesystem::path &plugin_dir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(plugin_dir, ec)) {
        UME_LOG_INFO(Plugin, "no plugin directory at '{}'",
                     plugin_dir.string());
        return;
    }

    for (const auto &entry :
         std::filesystem::directory_iterator(plugin_dir, ec)) {
        if (!entry.is_directory(ec)) {
            continue;
        }

        const std::filesystem::path library_path =
            findPluginLibrary(entry.path());
        if (library_path.empty()) {
            UME_LOG_WARN(Plugin, "no plugin library found in '{}",
                         entry.path().string());
            continue;
        }

        if (!host.loadPlugin(library_path)) {
            UME_LOG_ERROR(Plugin, "failed to load plugin from '{}'",
                          library_path.string());
            continue;
        }
    }

    if (ec) {
        UME_LOG_ERROR(Plugin, "error while scanning plugin directory: {}",
                      ec.message());
    }
}

void registerBuiltinPlugins(PluginHost &host,
                            const std::filesystem::path &plugin_dir) {
#ifdef UME_PLUGIN_STATIC
    if (!host.registerStatic("proc_planet", &UME_PLUGIN_ENTRY(procPlanet))) {
        UME_LOG_ERROR(Plugin, "error registering plugin 'proc_planet'");
    }
#else
    loadPluginsFrom(host, plugin_dir);
#endif
}
} // namespace ume