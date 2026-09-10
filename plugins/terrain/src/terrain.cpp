#include "terrain.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <iostream>

namespace terrain {
const uint64_t kLevelBits = 4;
const uint64_t kCoordBits = 30;
constexpr uint64_t kLevelMask = (15ULL << 2ULL * kCoordBits);
const uint64_t kCoordMask = (1ULL << kCoordBits) - 1ULL;

Terrain::Terrain(const TerrainPlugin *plugin) : plugin_(plugin) { generate(); }

void Terrain::generate() {
    // generate all chunks (just for testing)
    // for (uint64_t level = 0; level < kNumDetailLevels; level++) {
    //     const uint32_t res = 1UL << level;

    //     for (uint32_t i = 0; i < res; i++) {
    //         for (uint32_t j = 0; j < res; j++) {
    //             generateChunk(level, i, j);
    //         }
    //     }
    // }

    generateChunk(0, 0, 0);

    const UmePluginApi &api = plugin_->api;
    api.log(api.context, UME_LOG_LEVEL_INFO, "generating terrain");
}

void Terrain::update(const UmeFrameContext *frame_context) {
    const UmePluginApi &api = plugin_->api;

    glm::mat4 local_transform(1.0f);

    glm::dvec3 camera_position(frame_context->camera_position[0],
                               frame_context->camera_position[1],
                               frame_context->camera_position[2]);

    for (auto &[id, chunk] : chunks_) {
        const double size = size_ / (1 << chunk.level);
        const double origin = -size_ / 2.0;

        const double wx = origin + ((chunk.x + 0.5) * size);
        const double wz = origin + ((chunk.y + 0.5) * size);
        glm::dvec3 world_pos(wx, 0.0, wz);

        const double dist = glm::length(world_pos - camera_position);

        if (dist < size && chunk.level < kNumDetailLevels) {
            chunk.resident = false;

            generateChunk(chunk.level + 1, 2 * chunk.x, 2 * chunk.y);
            generateChunk(chunk.level + 1, (2 * chunk.x) + 1, 2 * chunk.y);
            generateChunk(chunk.level + 1, (2 * chunk.x) + 1,
                          (2 * chunk.y) + 1);
            generateChunk(chunk.level + 1, 2 * chunk.x, (2 * chunk.y) + 1);
        }

        if (chunk.resident) {
            api.submit(api.context, chunk.mesh, glm::value_ptr(world_pos),
                       glm::value_ptr(local_transform));
        }

        // std::cout << "updating chunk " << id << "\n";
        // std::cout << "distance: " << dist << "\n";
        // std::cout << "\n";
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

    const double inv_res = 1.0 / kChunkResolution;
    const uint32_t grid_width = kChunkResolution + 1;
    const size_t vertex_count = static_cast<size_t>(grid_width) * grid_width;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);

    for (uint32_t i = 0; i <= kChunkResolution; i++) {
        const double vz = (-size / 2.0) + (i * size * inv_res);
        for (uint32_t j = 0; j <= kChunkResolution; j++) {
            const double vx = (-size / 2.0) + (j * size * inv_res);

            const size_t k = (static_cast<size_t>(i) * grid_width) + j;
            const size_t base_idx = k * 3;

            positions[base_idx + 0] = static_cast<float>(vx);
            positions[base_idx + 1] = 0;
            positions[base_idx + 2] = static_cast<float>(vz);

            normals[base_idx + 0] = 0;
            normals[base_idx + 1] = 1;
            normals[base_idx + 2] = 0;
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(kChunkResolution) * kChunkResolution *
                    6);
    for (uint32_t i = 0; i < kChunkResolution; i++) {
        for (uint32_t j = i * grid_width;
             j < (i * grid_width) + kChunkResolution; j++) {
            indices.insert(indices.end(), {j + 1, j, j + grid_width});
            indices.insert(indices.end(),
                           {j + 1, j + grid_width, j + grid_width + 1});
        }
    }

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
                .mesh = api.createMesh(api.context, &desc),
                .resident = true};

    chunks_[id] = chunk;
}
} // namespace terrain