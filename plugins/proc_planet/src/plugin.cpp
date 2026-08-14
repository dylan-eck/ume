#include "ume/plugin/plugin_api.h"
#include "proc_planet.hpp"

namespace {

struct ProcPlanetPlugin {
    UmePluginApi api;
};

void *createPlanet(void *user_data, const UmeParams *params) {
    auto *self = static_cast<ProcPlanetPlugin *>(user_data);
    // const double radius = params->number(params->impl, "radius", 7000000.0);
    return new proc_planet::Planet(&self->api);
}

void destroyPlanet(void *user_data, void *object) {
    delete static_cast<proc_planet::Planet *>(object);
}

// TODO: change frame_context to const reference?
void updatePlanet(void *user_data, void *object,
                  const UmeFrameContext *frame_context) {
    auto *planet = static_cast<proc_planet::Planet *>(object);
    planet->update(frame_context);
}

void shutDownProcPlanet(void *state) {
    delete static_cast<ProcPlanetPlugin *>(state);
}
} // namespace

extern "C" UME_PLUGIN_EXPORT UME_PLUGIN_BOOL UME_PLUGIN_ENTRY(procPlanet)(
    const UmePluginApi *api, UmePluginDescription *description) {

    if (api->struct_size < sizeof(UmePluginApi)) {
        return UME_FALSE;
    }

    if (description->struct_size < sizeof(UmePluginDescription)) {
        return UME_FALSE;
    }

    auto *self = new ProcPlanetPlugin{*api};

    const UmeObjectType planet_type{
        .struct_size = sizeof(UmeObjectType),
        .name = "Planet",
        .user_data = self,
        .create = &createPlanet,
        .destroy = &destroyPlanet,
        .update = &updatePlanet,
    };

    if (api->registerObjectType(api->context, &planet_type) == UME_FALSE) {
        delete self;
        return UME_FALSE;
    }

    description->abi_version = UME_PLUGIN_ABI_VERSION;
    description->name = "proc_planet";
    description->state = self;
    description->shutdown = &shutDownProcPlanet;

    return UME_TRUE;
}