#pragma once
#include "ume/plugin/plugin_api.h"

#include <glm/glm.hpp>
#include <FastNoise/FastNoise.h>

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
    uint64_t last_used_frame;
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

    double size_ = 7e6;
    glm::dvec3 world_position_ = glm::dvec3(0.0);

    static constexpr uint64_t kNumDetailLevels = 18;
    static constexpr uint32_t kChunkResolution = 32;

    static constexpr uint32_t kMaxLoadedChunks = 512;
    static constexpr uint32_t kEvictionLowWaterMark = 400;

    FastNoise::SmartNode<FastNoise::Simplex> simplex_noise_;
    FastNoise::SmartNode<FastNoise::FractalFBm> fractal_noise_;

    std::unordered_map<uint64_t, Chunk> loaded_chunks_;
    std::vector<uint64_t> visible_chunks_ids_;

    Chunk &getOrCreateChunk(uint64_t level, uint32_t x, uint32_t y);

    void getVisibleChunkIds(uint64_t level, uint32_t x, uint32_t y,
                            const glm::dvec3 &camera_position);

    struct ChunkBounds {
        glm::dvec3 center;
        double size;
    };
    [[nodiscard]] ChunkBounds chunkBounds(uint64_t level, uint32_t x,
                                          uint32_t y) const {

        const double size = size_ / (1 << level);
        const double origin = -size_ / 2.0;

        const double wx = origin + ((x + 0.5) * size);
        const double wz = origin + ((y + 0.5) * size);
        glm::dvec3 world_pos(wx, 0.0, wz);

        return {.center = world_pos, .size = size};
    }
};
} // namespace terrain