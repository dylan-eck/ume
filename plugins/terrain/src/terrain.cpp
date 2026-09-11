#include "terrain.hpp"

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <iostream>

namespace terrain {
const uint64_t kCoordBits = 29;
const uint64_t kCoordMask = (1ULL << kCoordBits) - 1ULL;

namespace {
uint64_t chunkId(uint64_t level, uint32_t x, uint32_t y) {
    uint64_t id = level << 2ULL * kCoordBits;
    id |= static_cast<uint64_t>(x) << kCoordBits;
    id |= static_cast<uint64_t>(y);

    return id;
}

struct ChunkCoords {
    uint64_t level;
    uint32_t x;
    uint32_t y;
};

ChunkCoords chunkCoords(uint64_t id) {
    ChunkCoords coords;
    coords.level = id >> 2ULL * kCoordBits;
    coords.x = (id >> kCoordBits) & kCoordMask;
    coords.y = id & kCoordMask;

    return coords;
}
} // namespace

Terrain::Terrain(const TerrainPlugin *plugin) : plugin_(plugin) { generate(); }

void Terrain::generate() {
    getOrCreateChunk(0, 0, 0);

    const UmePluginApi &api = plugin_->api;
    api.log(api.context, UME_LOG_LEVEL_INFO, "generating terrain");
}

void Terrain::update(const UmeFrameContext *frame_context) {
    const UmePluginApi &api = plugin_->api;

    glm::dvec3 camera_position(frame_context->camera_position[0],
                               frame_context->camera_position[1],
                               frame_context->camera_position[2]);

    visible_chunks_ids_.clear();
    getVisibleChunkIds(0, 0, 0, camera_position);

    uint64_t max_lod = 0;

    for (const auto &id : visible_chunks_ids_) {
        ChunkCoords coords = chunkCoords(id);
        ChunkBounds bounds = chunkBounds(coords.level, coords.x, coords.y);

        max_lod = std::max(max_lod, coords.level);

        Chunk &chunk = getOrCreateChunk(coords.level, coords.x, coords.y);
        chunk.last_used_frame = frame_context->frame_number;

        api.submit(api.context, chunk.mesh, glm::value_ptr(bounds.center),
                   glm::value_ptr(glm::mat4(1.0f)));
    }

    std::cout << "=== frame " << frame_context->frame_number << "\n";
    std::cout << "      loaded chunks: " << loaded_chunks_.size() << "\n"
              << "     visible chunks: " << visible_chunks_ids_.size() << "\n"
              << "            max lod: " << max_lod << "\n\n";

    if (loaded_chunks_.size() <= kMaxLoadedChunks) {
        return;
    }

    if (visible_chunks_ids_.size() > kMaxLoadedChunks) {
        api.log(api.context, UME_LOG_LEVEL_WARN,
                "number of visible chunks exceeds loaded chunk limit");
        return;
    }

    std::vector<std::pair<uint64_t, uint64_t>> eviction_candidates;
    for (const auto &[id, chunk] : loaded_chunks_) {
        if (chunk.last_used_frame != frame_context->frame_number) {
            eviction_candidates.emplace_back(id, chunk.last_used_frame);
        }
    }

    const uint32_t evict_count = loaded_chunks_.size() - kEvictionLowWaterMark;

    const auto comp = [](std::pair<uint64_t, uint64_t> a,
                         std::pair<uint64_t, uint64_t> b) {
        return a.second < b.second;
    };

    std::nth_element(eviction_candidates.begin(),
                     eviction_candidates.begin() + evict_count,
                     eviction_candidates.end(), comp);

    for (uint32_t i = 0; i < evict_count; i++) {
        if (i >= eviction_candidates.size()) {
            break;
        }

        Chunk chunk = loaded_chunks_[eviction_candidates[i].first];
        loaded_chunks_.erase(eviction_candidates[i].first);
        api.destroyMesh(api.context, chunk.mesh);
    }
}

Chunk &Terrain::getOrCreateChunk(uint64_t level, uint32_t x, uint32_t y) {
    uint64_t id = chunkId(level, x, y);

    if (auto it = loaded_chunks_.find(id); it != loaded_chunks_.end()) {
        return loaded_chunks_[id];
    }

    const double size = chunkBounds(level, x, y).size;

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
                .mesh = api.createMesh(api.context, &desc)};

    loaded_chunks_[id] = chunk;
    return loaded_chunks_[id];
}

void Terrain::getVisibleChunkIds(uint64_t level, uint32_t x, uint32_t y,
                                 const glm::dvec3 &camera_position) {

    const ChunkBounds bounds = chunkBounds(level, x, y);

    const double dist = glm::length(bounds.center - camera_position);

    if (dist < bounds.size && level < kNumDetailLevels) {
        getVisibleChunkIds(level + 1, 2 * x, 2 * y, camera_position);
        getVisibleChunkIds(level + 1, (2 * x) + 1, 2 * y, camera_position);
        getVisibleChunkIds(level + 1, (2 * x) + 1, (2 * y) + 1,
                           camera_position);
        getVisibleChunkIds(level + 1, 2 * x, (2 * y) + 1, camera_position);
        return;
    }

    uint64_t id = level << 2 * kCoordBits;
    id |= static_cast<uint64_t>(x) << kCoordBits;
    id |= static_cast<uint64_t>(y);

    visible_chunks_ids_.push_back(id);
}
} // namespace terrain