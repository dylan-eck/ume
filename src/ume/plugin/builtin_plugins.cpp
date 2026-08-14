#include "ume/plugin/plugin_host.hpp"
#include "ume/core/logger.hpp"

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
    (void)host;
#endif
}
} // namespace ume

namespace ume {}