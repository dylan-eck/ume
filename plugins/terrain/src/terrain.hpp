#pragma once
#include "ume/plugin/plugin_api.h"

#include <glm/glm.hpp>

#include <unordered_map>

struct TerrainPlugin {
    UmePluginApi api;
};

namespace terrain {
struct Chunk {
    uint64_t level;
    uint32_t x;
    uint32_t y;
    UmeMeshHandle mesh;
};

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

    uint32_t resolution_ = 32;
    double size_ = 1024.0;
    glm::dvec3 world_position_ = glm::dvec3(0.0);

    static const uint64_t kNumDetailLevels = 3;

    UmeMeshHandle mesh_;

    std::unordered_map<uint64_t, Chunk> chunks_;

    void generateChunk(uint64_t level, uint32_t x, uint32_t y);
};
} // namespace terrain