#pragma once
#include "ume/plugin/plugin_api.h"

#include <glm/glm.hpp>

struct TerrainPlugin {
    UmePluginApi api;
};

namespace terrain {
class Terrain {
public:
    explicit Terrain(const TerrainPlugin *plugin);
    ~Terrain() = default;

    Terrain(const Terrain &) = delete;
    Terrain &operator=(const Terrain &) = delete;

    Terrain(Terrain &&) = delete;
    Terrain &operator=(Terrain &&) = delete;

    void generate();
    void update(const UmeFrameContext *frame_context);

private:
    const TerrainPlugin *plugin_;

    glm::dvec3 world_position_ = glm::dvec3(0.0);

    UmeMeshHandle mesh_;
};
} // namespace terrain