#include "terrain.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>

namespace terrain {
const uint64_t kLevelBits = 4;
const uint64_t kCoordBits = 30;
constexpr uint64_t kLevelMask = (15ULL << 2ULL * kCoordBits);
const uint64_t kCoordMask = (1ULL << kCoordBits) - 1ULL;

Terrain::Terrain(const TerrainPlugin *plugin) : plugin_(plugin) { generate(); }

void Terrain::generate() {
    // generate all chunks (just for testing)
    for (uint64_t level = 0; level < kNumDetailLevels; level++) {
        const uint32_t res = 1UL << level;

        for (uint32_t i = 0; i < res; i++) {
            for (uint32_t j = 0; j < res; j++) {
                generateChunk(level, i, j);
            }
        }
    }

    const UmePluginApi &api = plugin_->api;
    api.log(api.context, UME_LOG_LEVEL_INFO, "generating terrain");
}

void Terrain::update(const UmeFrameContext *frame_context) {
    const UmePluginApi &api = plugin_->api;

    glm::mat4 local_transform(1.0f);

    glm::dvec3 camera_position(frame_context->camera_position[0],
                               frame_context->camera_position[1],
                               frame_context->camera_position[2]);

    for (const auto &[id, chunk] : chunks_) {
        const double size = size_ / (1 << chunk.level);
        const double origin = -size_ / 2.0;

        const double wx = origin + ((chunk.x + 0.5) * size);
        const double wz = origin + ((chunk.y + 0.5) * size);

        const float h = static_cast<float>(chunk.level) * 256.0f;

        glm::dvec3 world_pos(wx, h, wz);

        api.submit(api.context, chunk.mesh, glm::value_ptr(world_pos),
                   glm::value_ptr(local_transform));
    }
}

void Terrain::generateChunk(uint64_t level, uint32_t x, uint32_t y) {
    uint64_t id = level << 2 * kCoordBits;
    id |= static_cast<uint64_t>(x) << kCoordBits;
    id |= static_cast<uint64_t>(y);

    if (auto it = chunks_.find(id); it != chunks_.end()) {
        return;
    }

    const double size = size_ / (1 << level);

    std::array<float, 12> positions{{0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, -0.5f,
                                     0.0f, -0.5f, 0.5f, 0.0f, -0.5f}};

    for (auto &v : positions) {
        v *= static_cast<float>(size);
    }

    std::array<float, 12> normals{{0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                   1.0f, 0.0f, 0.0f, 1.0f, 0.0f}};

    std::array<uint32_t, 6> indices{0, 2, 1, 0, 3, 2};

    UmeMeshDescription desc{
        .struct_size = sizeof(UmeMeshDescription),
        .vertex_count = static_cast<uint32_t>(positions.size()) / 3,
        .positions = positions.data(),
        .normals = normals.data(),
        .index_count = static_cast<uint32_t>(indices.size()),
        .indices = indices.data(),
    };

    const UmePluginApi &api = plugin_->api;

    Chunk chunk{.level = level,
                .x = x,
                .y = y,
                .mesh = api.createMesh(api.context, &desc)};

    chunks_[id] = chunk;
}
} // namespace terrain