#include "proc_planet.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <cmath>

namespace proc_planet {

Planet::Planet(const UmePluginApi *api, double radius)
    : api_(api), radius_(radius) {
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
    meshes_.clear();
    meshes_.reserve(6);

    uint32_t resolution = 8;

    const std::array<glm::dvec3, 6> face_normals{{
        glm::dvec3(0.0f, 0.0f, 1.0f),
        glm::dvec3(0.0f, 0.0f, -1.0f),
        glm::dvec3(1.0f, 0.0f, 0.0f),
        glm::dvec3(-1.0f, 0.0f, 0.0f),
        glm::dvec3(0.0f, 1.0f, 0.0f),
        glm::dvec3(0.0f, -1.0f, 0.0f),
    }};

    double step = 1.0 / resolution;
    for (const auto &normal : face_normals) {
        std::vector<UmeVertex> vertices;
        std::vector<uint32_t> indices;

        glm::dvec3 a = glm::dvec3(normal.y, normal.z, normal.x);
        glm::dvec3 b = glm::cross(normal, a);

        a *= 2.0f;
        b *= 2.0f;

        for (int i = 0; i < resolution; i++) {
            for (int j = 0; j < resolution; j++) {
                double t = (i * step) - 0.5;
                double u = (j * step) - 0.5;

                glm::dvec3 u00 = cubeToSphere(normal + a * t + b * u);
                glm::dvec3 u10 = cubeToSphere(normal + a * (t + step) + b * u);
                glm::dvec3 u11 =
                    cubeToSphere(normal + a * (t + step) + b * (u + step));
                glm::dvec3 u01 = cubeToSphere(normal + a * t + b * (u + step));

                glm::dvec3 p00 = u00 * radius_;
                glm::dvec3 p10 = u10 * radius_;
                glm::dvec3 p11 = u11 * radius_;
                glm::dvec3 p01 = u01 * radius_;

                glm::dvec3 n00 = glm::normalize(u00);
                glm::dvec3 n10 = glm::normalize(u10);
                glm::dvec3 n11 = glm::normalize(u11);
                glm::dvec3 n01 = glm::normalize(u01);

                size_t s = vertices.size();

                vertices.emplace_back(
                    UmeVertex{.position = {(float)p00[0], (float)p00[1],
                                           (float)p00[2], 1.0f},
                              .normal = {(float)n00[0], (float)n00[1],
                                         (float)n00[2], 0.0f}});

                vertices.emplace_back(
                    UmeVertex{.position = {(float)p10[0], (float)p10[1],
                                           (float)p10[2], 1.0f},
                              .normal = {(float)n10[0], (float)n10[1],
                                         (float)n10[2], 0.0f}});

                vertices.emplace_back(
                    UmeVertex{.position = {(float)p11[0], (float)p11[1],
                                           (float)p11[2], 1.0f},
                              .normal = {(float)n11[0], (float)n11[1],
                                         (float)n11[2], 0.0f}});

                vertices.emplace_back(
                    UmeVertex{.position = {(float)p01[0], (float)p01[1],
                                           (float)p01[2], 1.0f},
                              .normal = {(float)n01[0], (float)n01[1],
                                         (float)n01[2], 0.0f}});

                indices.push_back(s);
                indices.push_back(s + 1);
                indices.push_back(s + 2);

                indices.push_back(s + 2);
                indices.push_back(s + 3);
                indices.push_back(s);
            }
        }

        UmeMeshDescription desc{
            .struct_size = sizeof(UmeMeshDescription),
            .vertices = vertices.data(),
            .vertex_count = static_cast<uint32_t>(vertices.size()),
            .indices = indices.data(),
            .index_count = static_cast<uint32_t>(indices.size()),
        };

        UmeMeshHandle handle = api_->createMesh(api_->context, &desc);

        if (handle != UME_MESH_HANDLE_INVALID) {
            meshes_.emplace_back(api_, handle);
        }
    }
}

void Planet::update(const UmeFrameContext *frame_context) {
    for (const auto &ref : meshes_) {
        if (ref.getHandle() == UME_MESH_HANDLE_INVALID) {
            continue;
        }

        transform_ = glm::rotate(transform_, 0.0005f, glm::vec3(0, 1, 0));

        const std::array<double, 3> world_position = {0.0, 0.0, 0.0};
        api_->submit(api_->context, ref.getHandle(), world_position.data(),
                     glm::value_ptr(transform_));
    }
}
} // namespace proc_planet