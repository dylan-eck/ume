#include "ume/plugin/plugin_host.hpp"

#ifdef UME_PLUGIN_STATIC
extern "C" bool UME_PLUGIN_ENTRY(procPlanet)(const UmePluginApi *api,
                                             UmePluginDescription *description);
#endif

namespace ume {
void registerBuiltinPlugins(PluginHost &host) {
#ifdef UME_PLUGIN_STATIC
    host.registerStatic("proc_planet", &UME_PLUGIN_ENTRY(procPlanet));
#else
    (void)host;
#endif
}
} // namespace ume

namespace ume {}