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
    double size_ = 7e6;
    glm::dvec3 world_position_ = glm::dvec3(0.0);

    const uint64_t kNumDetailLevels = 8;
    const uint32_t kChunkResolution = 32;

    UmeMeshHandle mesh_;

    std::unordered_map<uint64_t, Chunk> chunks_;

    const Chunk &generateChunk(uint64_t level, uint32_t x, uint32_t y);
    void selectVisibleChunks(uint64_t level, uint32_t, uint32_t y,
                             const glm::dvec3 &camera_position);

    // [[nodiscard]] const Chunk &getChunk(uint64_t id) const;
};
} // namespace terrain