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
glm::vec3 cubeToSphere(glm::vec3 p) {
    return glm::vec3{p.x * std::sqrt(1 - (p.y * p.y / 2) - (p.z * p.z / 2) +
                                     (p.y * p.y * p.z * p.z / 3)),
                     p.y * std::sqrt(1 - (p.z * p.z / 2) - (p.x * p.x / 2) +
                                     (p.z * p.z * p.x * p.x / 3)),
                     p.z * std::sqrt(1 - (p.x * p.x / 2) - (p.y * p.y / 2) +
                                     (p.x * p.x * p.y * p.y / 3))};
}
} // namespace

void Planet::generate() {
    meshes_.clear();
    meshes_.reserve(6);

    uint32_t resolution = 8;

    const std::array<glm::vec3, 6> face_normals{{
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
    }};

    float step = 1.0f / resolution;
    for (const auto &normal : face_normals) {
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<uint32_t> indices;

        glm::vec3 a = 2.0f * glm::vec3(normal.y, normal.z, normal.x);
        glm::vec3 b = 2.0f * glm::cross(normal, a);

        auto push_vertex = [&](float t, float u) {
            const glm::vec3 dir = cubeToSphere(normal + a * t + b * u);
            const glm::vec3 p = dir * (float)radius_;
            const glm::vec3 n = glm::normalize(dir);

            positions.insert(positions.end(), {p.x, p.y, p.z});
            normals.insert(normals.end(), {n.x, n.y, n.z});
        };

        for (uint32_t i = 0; i < resolution; i++) {
            for (uint32_t j = 0; j < resolution; j++) {
                float t = (i * step) - 0.5f;
                float u = (j * step) - 0.5f;

                const auto s = static_cast<uint32_t>(positions.size()) / 3;

                push_vertex(t, u);
                push_vertex(t + step, u);
                push_vertex(t + step, u + step);
                push_vertex(t, u + step);

                indices.insert(indices.end(),
                               {s, s + 1, s + 2, s + 2, s + 3, s});
            }
        }

        auto simplex = FastNoise::New<FastNoise::Simplex>();
        auto fractal = FastNoise::New<FastNoise::FractalFBm>();
        fractal->SetSource(simplex);
        fractal->SetOctaveCount(5);

        const size_t vertex_count = positions.size() / 3;

        std::vector<float> x_positions(vertex_count);
        std::vector<float> y_positions(vertex_count);
        std::vector<float> z_positions(vertex_count);

        float freq = 0.00002f;

        for (size_t i = 0; i < vertex_count; i++) {
            const size_t base_idx = i * 3;

            x_positions[i] = positions[base_idx] * freq;
            y_positions[i] = positions[base_idx + 1] * freq;
            z_positions[i] = positions[base_idx + 2] * freq;
        }

        std::vector<float> noise_values(vertex_count);
        fractal->GenPositionArray3D(noise_values.data(),
                                    static_cast<int>(vertex_count),
                                    x_positions.data(), y_positions.data(),
                                    z_positions.data(), 0, 0, 0, 0);

        float amp = 800000.0f;

        for (size_t i = 0; i < vertex_count; i++) {
            float n = noise_values[i];
            float height = amp * n;

            const size_t base_idx = i * 3;

            positions[base_idx] += height * normals[base_idx];
            positions[base_idx + 1] += height * normals[base_idx + 1];
            positions[base_idx + 2] += height * normals[base_idx + 2];
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
            meshes_.emplace_back(api_, handle);
        }
    }
}

void Planet::update(const UmeFrameContext *frame_context) {

    rotation_angle_ += 0.25f * frame_context->delta_time;
    glm::mat4 transform =
        glm::rotate(glm::mat4(1.0f), rotation_angle_, glm::vec3(0, 1, 0));

    const std::array<double, 3> world_position = {
        world_position_.x, world_position_.y, world_position_.z};

    for (const auto &ref : meshes_) {
        if (ref.getHandle() == UME_MESH_HANDLE_INVALID) {
            continue;
        }

        api_->submit(api_->context, ref.getHandle(), world_position.data(),
                     glm::value_ptr(transform));
    }
}
} // namespace proc_planet