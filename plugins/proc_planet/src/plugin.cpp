#include "ume/plugin/plugin_api.h"

#include <iostream>

extern "C" UME_PLUGIN_EXPORT UME_PLUGIN_BOOL UME_PLUGIN_ENTRY(procPlanet)(
    const UmePluginApi *api, UmePluginDescription *description) {
    (void)api;
    (void)description;

    std::cout << "### registering proc_planet plugin! ###\n";

    return UME_FALSE;
}