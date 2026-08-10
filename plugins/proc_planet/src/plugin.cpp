#include "ume/plugin/plugin_api.h"
#include "proc_planet.hpp"

namespace {

struct ProcPlanetPlugin {
    UmePluginApi api;
};

void *createPlanet(void *user_data, const UmeParams *params) {
    auto *self = static_cast<ProcPlanetPlugin *>(user_data);
    // const double kRadius = params->number(params->impl, "radius", 7000000.0);
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
    auto *self = new ProcPlanetPlugin{*api};

    description->abi_version = 0;
    description->name = "proc_planet";
    description->state = self;
    description->shutdown = &shutDownProcPlanet;

    const UmeObjectType kPlanetType{
        .name = "Planet",
        .user_data = self,
        .create = &createPlanet,
        .destroy = &destroyPlanet,
        .update = &updatePlanet,
    };

    api->registerObjectType(api->context, &kPlanetType);

    return UME_TRUE;
}