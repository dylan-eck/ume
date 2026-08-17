#include "ume/plugin/plugin_host.hpp"
#include "ume/core/logger.hpp"

#include <filesystem>

#ifdef UME_PLUGIN_STATIC
extern "C" UME_PLUGIN_BOOL
    UME_PLUGIN_ENTRY(procPlanet)(const UmePluginApi *api,
                                 UmePluginDescription *description);
#endif

namespace ume {
void registerBuiltinPlugins(PluginHost &host) {
#ifdef UME_PLUGIN_STATIC
    if (!host.registerStatic("proc_planet", &UME_PLUGIN_ENTRY(procPlanet))) {
        UME_LOG_ERROR(Plugin, "error registering plugin 'proc_planet'");
    }
#else
    const std::filesystem::path &path("plugins/proc_planet/proc_planet.so");
    host.loadPlugin(path);
#endif
}
} // namespace ume