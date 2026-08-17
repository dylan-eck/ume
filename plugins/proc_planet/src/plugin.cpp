#include "ume/plugin/plugin_api.h"
#include "proc_planet.hpp"

#include <iostream>

namespace {

struct ProcPlanetPlugin {
    UmePluginApi api;
};

void *createPlanet(void *user_data, const UmeParams *params) noexcept {
    auto *self = static_cast<ProcPlanetPlugin *>(user_data);

    proc_planet::Planet *planet = nullptr;
    try {
        planet = new proc_planet::Planet(&self->api);
    } catch (...) {
        self->api.log(
            self->api.context, UME_LOG_LEVEL_ERROR,
            "proc_planet: unknown exception thrown during planet creation");
    }

    return planet;
}

void destroyPlanet(void *user_data, void *object) noexcept {
    auto *self = static_cast<ProcPlanetPlugin *>(user_data);
    try {
        delete static_cast<proc_planet::Planet *>(object);
    } catch (...) {
        if (self != nullptr) {
            self->api.log(self->api.context, UME_LOG_LEVEL_ERROR,
                          "proc_planet: unknown exception thrown during "
                          "planet destruction");
        }
    }
}

// TODO: change frame_context to const reference?
void updatePlanet(void *user_data, void *object,
                  const UmeFrameContext *frame_context) noexcept {

    auto *self = static_cast<ProcPlanetPlugin *>(user_data);
    auto *planet = static_cast<proc_planet::Planet *>(object);

    if (planet == nullptr) {
        return;
    }

    try {
        planet->update(frame_context);
    } catch (...) {
        self->api.log(
            self->api.context, UME_LOG_LEVEL_ERROR,
            "proc_planet: unknown exception thrown during planet update");
    }
}

void shutDownProcPlanet(void *state) noexcept {
    auto *self = static_cast<ProcPlanetPlugin *>(state);
    if (self == nullptr) {
        return;
    }

    const UmePluginApi api = self->api;

    try {
        delete static_cast<ProcPlanetPlugin *>(state);
    } catch (...) {
        api.log(api.context, UME_LOG_LEVEL_ERROR,
                "proc_planet: unknown exception thrown during plugin shutdown");
    }
}
} // namespace

extern "C" UME_PLUGIN_EXPORT UME_PLUGIN_BOOL UME_PLUGIN_ENTRY(procPlanet)(
    const UmePluginApi *api, UmePluginDescription *description) noexcept {

    ProcPlanetPlugin *self = nullptr;

    try {
        if (api->struct_size < sizeof(UmePluginApi)) {
            return UME_FALSE;
        }

        if (description->struct_size < sizeof(UmePluginDescription)) {
            return UME_FALSE;
        }

        self = new ProcPlanetPlugin{*api};

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
    } catch (...) {
        delete self;

        api->log(api->context, UME_LOG_LEVEL_ERROR,
                 "proc_planet: unknown exception thrown during plugin "
                 "registration");
        return UME_FALSE;
    }

    return UME_TRUE;
}