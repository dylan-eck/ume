#include "ume/plugin/plugin_api.h"

extern "C" UME_PLUGIN_EXPORT bool
UME_PLUGIN_ENTRY(procPlanet)(const UmePluginApi *api,
                             UmePluginDescription *description) {
    (void)api;
    (void)description;

    return false;
}