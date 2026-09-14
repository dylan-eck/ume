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

Terrain::Terrain(const TerrainPlugin *plugin)
    : plugin_(plugin), simplex_noise_(FastNoise::New<FastNoise::Simplex>()),
      fractal_noise_(FastNoise::New<FastNoise::FractalFBm>()) {
    fractal_noise_->SetSource(simplex_noise_);
    // fractal_noise_->SetOctaveCount(4);
    fractal_noise_->SetGain(0.5f);

    generate();
}

void Terrain::generate() {
    getOrCreateChunk(0, 0, 0);

    const UmePluginApi &api = plugin_->api;
    api.log(api.context, UME_LOG_LEVEL_INFO, "generating terrain");
}

void Terrain::update(const UmeFrameContext *frame_context) {
    const UmePluginApi &api = plugin_->api;

    // std::cout << "camera altitude: " << frame_context->camera_position[1]
    //           << "\n";

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

    // std::cout << "=== frame " << frame_context->frame_number << "\n";
    // std::cout << "      loaded chunks: " << loaded_chunks_.size() << "\n"
    //           << "     visible chunks: " << visible_chunks_ids_.size() <<
    //           "\n"
    //           << "            max lod: " << max_lod << "\n\n";

    if (loaded_chunks_.size() <= kMaxLoadedChunks) return;

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
        if (i >= eviction_candidates.size()) break;

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

    const ChunkBounds bounds = chunkBounds(level, x, y);
    const double size = bounds.size;
    const glm::dvec3 chunk_world_position = bounds.center;

    const double noise_freq =
        1.0 / 25000.0; // 1.5e6; // 1 / feature wavelength in meters
    const float noise_amp = 80000.0f; // +/- height variation in meters

    const double inv_res = 1.0 / kChunkResolution;
    const uint32_t grid_width = kChunkResolution + 3;
    const size_t vertex_count = static_cast<size_t>(grid_width) * grid_width;

    std::vector<float> positions(vertex_count * 3);
    std::vector<float> normals(vertex_count * 3);

    std::vector<float> noise_input_x(vertex_count);
    std::vector<float> noise_input_z(vertex_count);

    for (uint32_t i = 0; i < grid_width; i++) {
        const double vz =
            (-size / 2.0) - (size * inv_res) + (i * size * inv_res);
        for (uint32_t j = 0; j < grid_width; j++) {
            const double vx =
                (-size / 2.0) - (size * inv_res) + (j * size * inv_res);

            const size_t k = (static_cast<size_t>(i) * grid_width) + j;
            const size_t base_idx = k * 3;

            positions[base_idx + 0] = static_cast<float>(vx);
            positions[base_idx + 1] = 0;
            positions[base_idx + 2] = static_cast<float>(vz);

            noise_input_x[k] =
                static_cast<float>((vx + chunk_world_position.x) * noise_freq);
            noise_input_z[k] =
                static_cast<float>((vz + chunk_world_position.z) * noise_freq);
        }
    }

    std::vector<float> noise_values(vertex_count);
    fractal_noise_->SetOctaveCount(static_cast<int>(level + 1));
    FastNoise::OutputMinMax output_min_max = fractal_noise_->GenPositionArray2D(
        noise_values.data(), vertex_count, noise_input_x.data(),
        noise_input_z.data(), 0, 0, 0);

    // std::cout << "level: " << level
    //           << " noise output range: " << output_min_max.min << " "
    //           << output_min_max.max << "\n";

    for (size_t i = 0; i < vertex_count; i++) {
        const float height = noise_amp * noise_values[i];
        const size_t base_idx = i * 3;
        positions[base_idx + 1] += height;
    }

    auto p = [&](uint32_t i, uint32_t j) {
        const size_t base_idx = (size_t(i) * grid_width + j) * 3;
        return glm::vec3(positions[base_idx], positions[base_idx + 1],
                         positions[base_idx + 2]);
    };

    for (uint32_t i = 1; i <= kChunkResolution + 1; i++) {
        for (uint32_t j = 1; j <= kChunkResolution + 1; j++) {
            const glm::vec3 di = p(i + 1, j) - p(i - 1, j);
            const glm::vec3 dj = p(i, j + 1) - p(i, j - 1);

            glm::vec3 nrm = glm::cross(di, dj);
            const float len = glm::length(nrm);
            nrm = (len > 0.0f) ? nrm / len : glm::vec3(0.0f, 1.0f, 0.0f);

            const size_t base = (size_t(i) * grid_width + j) * 3;
            normals[base + 0] = nrm.x;
            normals[base + 1] = nrm.y;
            normals[base + 2] = nrm.z;
        }
    }

    for (uint32_t i = 0; i < grid_width; i++) {
        for (uint32_t j = 0; j < grid_width; j++) {
            if (i > 0 && i < grid_width - 1 && j > 0 && j < grid_width - 1) {
                continue;
            }

            const size_t base = (static_cast<size_t>(i) * grid_width + j) * 3;

            uint32_t ti = i;
            uint32_t tj = j;

            if (i == 0) {
                ti++;
            } else if (i == grid_width - 1) {
                ti--;
            }

            if (j == 0) {
                tj++;
            } else if (j == grid_width - 1) {
                tj--;
            }

            const size_t target =
                ((static_cast<size_t>(ti) * grid_width) + tj) * 3;

            positions[base + 0] = positions[target + 0];
            positions[base + 1] =
                positions[target + 1] -
                static_cast<float>(2.0 * bounds.size / kChunkResolution);
            positions[base + 2] = positions[target + 2];

            normals[base + 0] = normals[target + 0];
            normals[base + 1] = normals[target + 1];
            normals[base + 2] = normals[target + 2];
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(kChunkResolution) * kChunkResolution *
                    6);
    for (uint32_t i = 0; i < grid_width - 1; i++) {
        for (uint32_t j = (i * grid_width);
             j < (i * grid_width) + grid_width - 1; j++) {
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