#include "proc_planet.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <FastNoise/FastNoise.h>

#include <cmath>
#include <array>

namespace proc_planet {

Planet::Planet(const ProcPlanetPlugin *plugin, double radius,
               glm::dvec3 world_position)
    : plugin_(plugin), radius_(radius), world_position_(world_position) {
    generate();
}

namespace {
glm::dvec3 foldToCubeSurface(glm::dvec3 n, glm::dvec3 a, glm::dvec3 b, double t,
                             double u) {
    const bool t_out = std::abs(t) > 1.0;
    const bool u_out = std::abs(u) > 1.0;
    if (!t_out && !u_out) return n + a * t + b * u;

    const bool fold_t =
        t_out && (!u_out || std::abs(t) - 1.0 >= std::abs(u) - 1.0);

    glm::dvec3 n2;
    glm::dvec3 p_edge;

    double excess;
    if (fold_t) {
        const double sign = (t > 0.0) ? 1.0 : -1.0;
        excess = std::abs(t) - 1.0;
        n2 = sign * a;
        p_edge = n + a * sign + b * u;
    } else {
        const double sign = (u > 0.0) ? 1.0 : -1.0;
        excess = std::abs(u) - 1.0;
        n2 = sign * b;
        p_edge = n + a * t + b * sign;
    }
    const glm::dvec3 a2(n2.y, n2.z, n2.x);
    const glm::dvec3 b2 = glm::cross(n2, a2);

    double t2 = glm::dot(p_edge, a2);
    double u2 = glm::dot(p_edge, b2);

    if (std::abs(glm::dot(n, a2)) > 0.5) {
        t2 += (t2 >= 0.0 ? -excess : excess);
    } else {
        u2 += (u2 >= 0.0 ? -excess : excess);
    }

    return n2 + a2 * t2 + b2 * u2;
}

glm::dvec3 cubeToSphere(glm::dvec3 p) {
    return glm::dvec3{p.x * std::sqrt(1 - (p.y * p.y / 2) - (p.z * p.z / 2) +
                                      (p.y * p.y * p.z * p.z / 3)),
                      p.y * std::sqrt(1 - (p.z * p.z / 2) - (p.x * p.x / 2) +
                                      (p.z * p.z * p.x * p.x / 3)),
                      p.z * std::sqrt(1 - (p.x * p.x / 2) - (p.y * p.y / 2) +
                                      (p.x * p.x * p.y * p.y / 3))};
}

float smoothMax(float x, float y, float k) {
    return (1 / k) * std::log(std::exp(k * x) + std::exp(k * y));
}
} // namespace

void Planet::generate() {
    chunks_.clear();
    chunks_.reserve(6);

    const uint32_t resolution = 256;
    const double inv_res = 1.0 / resolution;
    const uint32_t grid_width = resolution + 3;
    const size_t vertex_count = static_cast<size_t>(grid_width) * grid_width;

    const double noise_freq = 1.0 / 2.0; // 1 / feature wavelength in meters
    const float noise_amp = 20.0f;       // +/- height variation in meters

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
        std::vector<float> normals(vertex_count * 3, 0.0f);
        std::vector<float> radials(vertex_count * 3);

        std::vector<float> noise_input_x(vertex_count);
        std::vector<float> noise_input_y(vertex_count);
        std::vector<float> noise_input_z(vertex_count);

        for (uint32_t i = 0; i < grid_width; i++) {
            const double t = (2.0 * i * inv_res) - (2.0 * inv_res) - 1.0;
            for (uint32_t j = 0; j < grid_width; j++) {
                const double u = (2.0 * j * inv_res) - (2.0 * inv_res) - 1.0;

                const glm::dvec3 folded = foldToCubeSurface(n, a, b, t, u);
                const glm::dvec3 dir = cubeToSphere(folded);
                const glm::dvec3 world_pos = dir * radius_;
                const glm::dvec3 local_pos = world_pos - face_origin;

                const size_t k = (size_t(i) * grid_width) + j;
                const size_t base_idx = k * 3;

                positions[base_idx + 0] = float(local_pos.x);
                positions[base_idx + 1] = float(local_pos.y);
                positions[base_idx + 2] = float(local_pos.z);

                radials[base_idx + 0] = float(dir.x);
                radials[base_idx + 1] = float(dir.y);
                radials[base_idx + 2] = float(dir.z);

                noise_input_x[k] = float(world_pos.x * noise_freq);
                noise_input_y[k] = float(world_pos.y * noise_freq);
                noise_input_z[k] = float(world_pos.z * noise_freq);
            }
        }

        std::vector<uint32_t> indices;
        indices.reserve(size_t(resolution) * resolution * 6);
        for (uint32_t i = 1; i <= resolution; i++) {
            for (uint32_t j = (i * grid_width) + 1;
                 j < (i * grid_width) + resolution + 1; j++) {
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
        // fractal->SetLacunarity(0.5);

        std::vector<float> noise_values(vertex_count);
        fractal->GenPositionArray3D(noise_values.data(),
                                    static_cast<int>(vertex_count),
                                    noise_input_x.data(), noise_input_y.data(),
                                    noise_input_z.data(), 0, 0, 0, 0);

        float ocean_floor_depth = 5.0f;
        float ocean_depth_multiplier = 1.0f;

        for (size_t k = 0; k < vertex_count; k++) {
            float offset = noise_values[k];
            offset = smoothMax(
                offset, (-ocean_floor_depth / noise_amp) + (offset * 0.15f),
                0.5f * noise_amp);

            float below = -smoothMax(-offset, 0.0f, 0.5f * noise_amp);
            offset += ocean_depth_multiplier * below;

            offset *= noise_amp;

            const size_t base_idx = k * 3;
            positions[base_idx + 0] += offset * radials[base_idx + 0];
            positions[base_idx + 1] += offset * radials[base_idx + 1];
            positions[base_idx + 2] += offset * radials[base_idx + 2];
        }

        auto p = [&](uint32_t i, uint32_t j) {
            const size_t base_idx = (size_t(i) * grid_width + j) * 3;
            return glm::vec3(positions[base_idx], positions[base_idx + 1],
                             positions[base_idx + 2]);
        };

        for (uint32_t i = 1; i <= resolution + 1; i++) {
            for (uint32_t j = 1; j <= resolution + 1; j++) {
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

        // TODO: remove "ghost" vertices in outer ring
        UmeMeshDescription desc{
            .struct_size = sizeof(UmeMeshDescription),
            .vertex_count = static_cast<uint32_t>(positions.size() / 3),
            .positions = positions.data(),
            .normals = normals.data(),
            .index_count = static_cast<uint32_t>(indices.size()),
            .indices = indices.data(),
        };

        UmeMeshHandle handle =
            plugin_->api.createMesh(plugin_->api.context, &desc);

        if (handle != UME_MESH_HANDLE_INVALID) {
            chunks_.emplace_back(Chunk{.mesh = MeshRef(&plugin_->api, handle),
                                       .local_origin = face_origin});
        }
    }
}

namespace {
struct OceanParams {
    // xyz: planet center relative to the camera, w: ocean radius
    float center_and_radius[4];
    // x: planet radius, y: atmosphere thickness, z: scattering strength (1/m)
    float planet_radius_thickness_strength[4];
};
static_assert(sizeof(OceanParams) == 32);

struct AtmosphereParams {
    // xyz: planet center relative to the camera, w: atmosphere radius
    float center_and_radius[4];
    // x: planet radius, y: atmosphere thickness, z: scattering strength (1/m)
    float planet_radius_thickness_strength[4];
};
static_assert(sizeof(AtmosphereParams) == 32);

// Atmosphere thickness as a fraction of the planet radius, so the atmosphere
// keeps the same proportions whatever the planet size.
constexpr double kAtmosphereThicknessRatio = 0.4;

// Dimensionless optical depth through the full atmosphere thickness at the
// most strongly scattered (blue) wavelength. The per-meter scattering strength
// is derived from this so the look is independent of planet scale.
constexpr double kAtmosphereOpticalDepth = 1.0;
} // namespace

void Planet::update(const UmeFrameContext *frame_context) {
    rotation_angle_ += 0.0f * frame_context->delta_time;
    rotation_angle_ = std::fmod(rotation_angle_, glm::two_pi<float>());

    const glm::dmat3 rotation{glm::rotate(
        glm::dmat4(1.0), double(rotation_angle_), glm::dvec3(0.0, 1.0, 0.0))};

    const glm::mat4 local_transform = glm::rotate(
        glm::mat4(1.0f), rotation_angle_, glm::vec3(0.0f, 1.0f, 0.0f));

    for (const auto &chunk : chunks_) {
        const MeshRef &mesh = chunk.mesh;
        if (mesh.getHandle() == UME_MESH_HANDLE_INVALID) continue;

        const glm::dvec3 world =
            world_position_ + rotation * chunk.local_origin;

        plugin_->api.submit(plugin_->api.context, mesh.getHandle(),
                            glm::value_ptr(world),
                            glm::value_ptr(local_transform));
    }

    const glm::dvec3 camera(frame_context->camera_position[0],
                            frame_context->camera_position[1],
                            frame_context->camera_position[2]);
    const glm::dvec3 rel = world_position_ - camera;

    const double atmosphere_thickness = radius_ * kAtmosphereThicknessRatio;
    const double scattering_strength =
        0.025; // kAtmosphereOpticalDepth / atmosphere_thickness;

    // if (plugin_->ocean != UME_POST_EFFECT_HANDLE_INVALID) {
    //     const OceanParams params{
    //         .center_and_radius = {float(rel.x), float(rel.y), float(rel.z),
    //                               float(radius_)},
    //         .planet_radius_thickness_strength = {
    //             float(radius_), float(atmosphere_thickness),
    //             float(scattering_strength), 0.0f}};

    //     plugin_->api.submitPostEffect(plugin_->api.context, plugin_->ocean,
    //                                   &params, sizeof(params));
    // }

    // if (plugin_->atmosphere != UME_POST_EFFECT_HANDLE_INVALID) {

    //     const AtmosphereParams params{
    //         .center_and_radius = {float(rel.x), float(rel.y), float(rel.z),
    //                               float(radius_ + atmosphere_thickness)},
    //         .planet_radius_thickness_strength = {
    //             float(radius_), float(atmosphere_thickness),
    //             float(scattering_strength), 0.0f}};

    //     plugin_->api.submitPostEffect(plugin_->api.context,
    //     plugin_->atmosphere,
    //                                   &params, sizeof(params));
    // }
}
} // namespace proc_planet