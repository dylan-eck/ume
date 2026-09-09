#include "ume/plugin/plugin_api.h"

#include "terrain.hpp"

namespace {

void *createTerrain(void *user_data, const UmeParams *params) noexcept {
    auto *self = static_cast<TerrainPlugin *>(user_data);

    terrain::Terrain *terrain = nullptr;
    try {
        terrain = new terrain::Terrain(self);
    } catch (...) {
        self->api.log(
            self->api.context, UME_LOG_LEVEL_ERROR,
            "terrain: unknown exception thrown during terrain creation");
    }

    return terrain;
}

// this function's signature is fixed by the plugin ABI (plugin_api.h)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void destroyTerrain([[maybe_unused]] void *user_data, void *object) noexcept {
    delete static_cast<terrain::Terrain *>(object);
}

// TODO: change frame_context to const reference?
// this function's signature is fixed by the plugin ABI (plugin_api.h)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void updateTerrain(void *user_data, void *object,
                   const UmeFrameContext *frame_context) noexcept {

    auto *self = static_cast<TerrainPlugin *>(user_data);
    auto *terrain = static_cast<terrain::Terrain *>(object);

    if (terrain == nullptr) {
        return;
    }

    try {
        terrain->update(frame_context);
    } catch (...) {
        self->api.log(
            self->api.context, UME_LOG_LEVEL_ERROR,
            "terrain: unknown exception thrown during terrain update");
    }
}

UME_PLUGIN_BOOL initTerrainPlugin(void *state) noexcept {
    auto *self = static_cast<TerrainPlugin *>(state);
    if (self == nullptr) {
        return UME_FALSE;
    }

    const UmePluginApi api = self->api;

    const UmeObjectType planet_type{
        .struct_size = sizeof(UmeObjectType),
        .name = "Terrain",
        .user_data = self,
        .create = &createTerrain,
        .destroy = &destroyTerrain,
        .update = &updateTerrain,
    };

    if (api.registerObjectType(api.context, &planet_type) == UME_FALSE) {
        return UME_FALSE;
    }

    return UME_TRUE;
}

void shutdownTerrainPlugin(void *state) noexcept {
    auto *self = static_cast<TerrainPlugin *>(state);
    if (self == nullptr) {
        return;
    }

    const UmePluginApi api = self->api;

    try {
        delete static_cast<TerrainPlugin *>(state);
    } catch (...) {
        api.log(api.context, UME_LOG_LEVEL_ERROR,
                "terrain: unknown exception thrown during plugin shutdown");
    }
}
} // namespace

extern "C" UME_PLUGIN_EXPORT UME_PLUGIN_BOOL UME_PLUGIN_ENTRY(terrain)(
    const UmePluginApi *api, UmePluginDescription *description) noexcept {

    TerrainPlugin *self = nullptr;

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

        self = new TerrainPlugin{*api};
        description->abi_version = UME_PLUGIN_ABI_VERSION;
        description->name = "terrain";
        description->state = self;
        description->init = &initTerrainPlugin;
        description->shutdown = &shutdownTerrainPlugin;
    } catch (...) {
        delete self;

        api->log(api->context, UME_LOG_LEVEL_ERROR,
                 "terrain: unknown exception thrown during plugin "
                 "registration");
        return UME_FALSE;
    }

    return UME_TRUE;
}