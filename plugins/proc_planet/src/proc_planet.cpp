#include "proc_planet.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <FastNoise/FastNoise.h>

#include <cmath>
#include <array>

namespace proc_planet {

Planet::Planet(const UmePluginApi *api, double radius,
               glm::dvec3 world_position)
    : api_(api), radius_(radius), world_position_(world_position) {
    generate();
}

namespace {
glm::dvec3 cubeToSphere(glm::dvec3 p) {
    return glm::dvec3{p.x * std::sqrt(1 - (p.y * p.y / 2) - (p.z * p.z / 2) +
                                      (p.y * p.y * p.z * p.z / 3)),
                      p.y * std::sqrt(1 - (p.z * p.z / 2) - (p.x * p.x / 2) +
                                      (p.z * p.z * p.x * p.x / 3)),
                      p.z * std::sqrt(1 - (p.x * p.x / 2) - (p.y * p.y / 2) +
                                      (p.x * p.x * p.y * p.y / 3))};
}
} // namespace

void Planet::generate() {
    chunks_.clear();
    chunks_.reserve(6);

    const uint32_t resolution = 32;
    const double inv_res = 1.0 / resolution;
    const uint32_t grid_width = resolution + 1;
    const size_t vertex_count = static_cast<size_t>(grid_width) * grid_width;

    const double noise_freq = 1.0 / 50000.0;  // 1 / feature wavelength
    const float noise_amp = 300.0f * 1000.0f; // +/- height variation in meters

    const std::array<glm::vec3, 6> face_normals{{
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
    }};

    for (const auto &face_normal : face_normals) {
        const glm::dvec3 n(face_normal);
        const glm::dvec3 a(n.y, n.z, n.x);
        const glm::dvec3 b = glm::cross(n, a);

        const glm::dvec3 face_origin = cubeToSphere(n) * radius_;

        std::vector<float> positions(vertex_count * 3);
        std::vector<float> normals(vertex_count * 3);

        std::vector<float> noise_input_x(vertex_count);
        std::vector<float> noise_input_y(vertex_count);
        std::vector<float> noise_input_z(vertex_count);

        for (uint32_t i = 0; i <= resolution; i++) {
            for (uint32_t j = 0; j <= resolution; j++) {
                const double t = (2.0 * i * inv_res) - 1.0;
                const double u = (2.0 * j * inv_res) - 1.0;

                const glm::dvec3 local_cube = a * t + b * u;

                const glm::dvec3 dir = cubeToSphere(n + local_cube);
                const glm::dvec3 world_pos = dir * radius_;
                const glm::dvec3 local_pos = world_pos - face_origin;

                const size_t k = (size_t(i) * grid_width) + j;
                const size_t base_idx = k * 3;

                positions[base_idx + 0] = float(local_pos.x);
                positions[base_idx + 1] = float(local_pos.y);
                positions[base_idx + 2] = float(local_pos.z);

                normals[base_idx + 0] = float(dir.x);
                normals[base_idx + 1] = float(dir.y);
                normals[base_idx + 2] = float(dir.z);

                noise_input_x[k] = float(world_pos.x * noise_freq);
                noise_input_y[k] = float(world_pos.y * noise_freq);
                noise_input_z[k] = float(world_pos.z * noise_freq);
            }
        }

        std::vector<uint32_t> indices;
        for (uint32_t i = 0; i < resolution; i++) {
            for (uint32_t j = i * grid_width; j < (i * grid_width) + resolution;
                 j++) {
                indices.insert(indices.end(), {j + 1, j, j + grid_width});
                indices.insert(indices.end(),
                               {j + 1, j + grid_width, j + grid_width + 1});
            }
        }

        auto simplex = FastNoise::New<FastNoise::Simplex>();
        simplex->SetOutputMin(-1.0f);
        simplex->SetOutputMax(1.0f);

        auto fractal = FastNoise::New<FastNoise::FractalFBm>();
        fractal->SetSource(simplex);
        fractal->SetOctaveCount(4);

        std::vector<float> noise_values(vertex_count);
        fractal->GenPositionArray3D(noise_values.data(),
                                    static_cast<int>(vertex_count),
                                    noise_input_x.data(), noise_input_y.data(),
                                    noise_input_z.data(), 0, 0, 0, 0);

        for (size_t i = 0; i < vertex_count; i++) {
            float n = noise_values[i];
            float height = noise_amp * n;

            const size_t base_idx = i * 3;
            positions[base_idx] += height * normals[base_idx];
            positions[base_idx + 1] += height * normals[base_idx + 1];
            positions[base_idx + 2] += height * normals[base_idx + 2];
        }

        std::ranges::fill(normals.begin(), normals.end(), 0.0f);

        for (size_t i = 0; i < indices.size(); i += 3) {
            const size_t i0 = size_t(indices[i + 0]) * 3;
            const size_t i1 = size_t(indices[i + 1]) * 3;
            const size_t i2 = size_t(indices[i + 2]) * 3;

            const glm::vec3 v0(positions[i0], positions[i0 + 1],
                               positions[i0 + 2]);
            const glm::vec3 v1(positions[i1], positions[i1 + 1],
                               positions[i1 + 2]);
            const glm::vec3 v2(positions[i2], positions[i2 + 1],
                               positions[i2 + 2]);

            const glm::vec3 face_normal = glm::cross(v1 - v0, v2 - v0);

            for (const size_t base : {i0, i1, i2}) {
                normals[base + 0] += face_normal.x;
                normals[base + 1] += face_normal.y;
                normals[base + 2] += face_normal.z;
            }
        }

        for (size_t k = 0; k < vertex_count; k++) {
            const size_t base = k * 3;
            glm::vec3 n(normals[base], normals[base + 1], normals[base + 2]);
            const float len = glm::length(n);
            n = (len > 0.0f) ? n / len : glm::vec3(0.0f, 1.0f, 0.0f);
            normals[base + 0] = n.x;
            normals[base + 1] = n.y;
            normals[base + 2] = n.z;
        }

        UmeMeshDescription desc{
            .struct_size = sizeof(UmeMeshDescription),
            .vertex_count = static_cast<uint32_t>(positions.size() / 3),
            .positions = positions.data(),
            .normals = normals.data(),
            .index_count = static_cast<uint32_t>(indices.size()),
            .indices = indices.data(),
        };

        UmeMeshHandle handle = api_->createMesh(api_->context, &desc);

        if (handle != UME_MESH_HANDLE_INVALID) {
            chunks_.emplace_back(Chunk{.mesh = MeshRef(api_, handle),
                                       .local_origin = face_origin});
        }
    }
}

void Planet::update(const UmeFrameContext *frame_context) {
    rotation_angle_ += 0.25f * frame_context->delta_time;
    rotation_angle_ = std::fmod(rotation_angle_, glm::two_pi<float>());

    const glm::dmat3 rotation{glm::rotate(
        glm::dmat4(1.0), double(rotation_angle_), glm::dvec3(0.0, 1.0, 0.0))};

    const glm::mat4 local_transform = glm::rotate(
        glm::mat4(1.0f), rotation_angle_, glm::vec3(0.0f, 1.0f, 0.0f));

    for (const auto &chunk : chunks_) {
        const MeshRef &mesh = chunk.mesh;
        if (mesh.getHandle() == UME_MESH_HANDLE_INVALID) {
            continue;
        }

        const glm::dvec3 world =
            world_position_ + rotation * chunk.local_origin;

        api_->submit(api_->context, mesh.getHandle(), glm::value_ptr(world),
                     glm::value_ptr(local_transform));
    }
}
} // namespace proc_planet