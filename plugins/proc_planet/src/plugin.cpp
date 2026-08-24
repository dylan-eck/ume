#include "ume/plugin/plugin_api.h"
#include "proc_planet.hpp"

namespace {

struct ProcPlanetPlugin {
    UmePluginApi api;
};

void *createPlanet(void *user_data, const UmeParams *params) noexcept {
    auto *self = static_cast<ProcPlanetPlugin *>(user_data);

    double radius = 1.0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (params != nullptr && params->struct_size >= sizeof(UmeParams) &&
        params->number != nullptr) {
        radius = params->number(params->impl, "radius", radius);
        x = params->number(params->impl, "x", x);
        y = params->number(params->impl, "y", y);
        z = params->number(params->impl, "z", z);
    }

    // the check is structured this way so that NaN is not a valid radius
    if (!(radius > 0.0)) {
        self->api.log(self->api.context, UME_LOG_LEVEL_WARN,
                      "proc_planet: radius is non-positive, using 1.0 instead");
        radius = 1.0;
    }

    proc_planet::Planet *planet = nullptr;
    try {
        planet =
            new proc_planet::Planet(&self->api, radius, glm::dvec3(x, y, z));
    } catch (...) {
        self->api.log(
            self->api.context, UME_LOG_LEVEL_ERROR,
            "proc_planet: unknown exception thrown during planet creation");
    }

    return planet;
}

// this function's signature is fixed by the plugin ABI (plugin_api.h)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void destroyPlanet([[maybe_unused]] void *user_data, void *object) noexcept {
    // destructors are implicitly noexcept, so no try catch needed here
    delete static_cast<proc_planet::Planet *>(object);
}

// TODO: change frame_context to const reference?
// this function's signature is fixed by the plugin ABI (plugin_api.h)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
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

UME_PLUGIN_BOOL initProcPlanet(void *state) noexcept {
    auto *self = static_cast<ProcPlanetPlugin *>(state);
    if (self == nullptr) {
        return UME_FALSE;
    }

    const UmePluginApi api = self->api;

    const UmeObjectType planet_type{
        .struct_size = sizeof(UmeObjectType),
        .name = "Planet",
        .user_data = self,
        .create = &createPlanet,
        .destroy = &destroyPlanet,
        .update = &updatePlanet,
    };

    if (api.registerObjectType(api.context, &planet_type) == UME_FALSE) {
        return UME_FALSE;
    }

    return UME_TRUE;
}

void shutdownProcPlanet(void *state) noexcept {
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
        // if abi version doesn't match, we can't call api-log in the catch
        // because function pointer locations are not guaranteed to match
        if (api->abi_version != UME_PLUGIN_ABI_VERSION) {
            return UME_FALSE;
        }

        if (api->struct_size < sizeof(UmePluginApi)) {
            return UME_FALSE;
        }

        if (description->struct_size < sizeof(UmePluginDescription)) {
            return UME_FALSE;
        }

        self = new ProcPlanetPlugin{*api};
        description->abi_version = UME_PLUGIN_ABI_VERSION;
        description->name = "proc_planet";
        description->state = self;
        description->init = &initProcPlanet;
        description->shutdown = &shutdownProcPlanet;
    } catch (...) {
        delete self;

        api->log(api->context, UME_LOG_LEVEL_ERROR,
                 "proc_planet: unknown exception thrown during plugin "
                 "registration");
        return UME_FALSE;
    }

    return UME_TRUE;
}