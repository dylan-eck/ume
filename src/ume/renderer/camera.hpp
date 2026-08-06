#pragma once

#include <glm/glm.hpp>

namespace ume {
struct CameraState {
    glm::dvec3 position{0.0};
    glm::mat3 orientation{1.0f};
    float fov_y = glm::radians(30.0f);
    float z_near = 1.0f;
};

inline CameraState lookAtCamera(const glm::dvec3 &position,
                                const glm::dvec3 &target, const glm::dvec3 &up,
                                float fov_y, float z_near) {
    const glm::dvec3 kForward = glm::normalize(target - position);
    const glm::dvec3 kRight = glm::normalize(glm::cross(kForward, up));
    const glm::dvec3 kUp = glm::cross(kRight, kForward);

    return CameraState{
        .position = position,
        .orientation =
            glm::mat3(glm::vec3(kRight), glm::vec3(kUp), glm::vec3(-kForward)),
        .fov_y = fov_y,
        .z_near = z_near};
}

inline glm::mat4 perspectiveReverseZ(float fov_y, float aspect, float z_near) {
    const float kF = 1.0f / std::tan(fov_y * 0.5f);
    auto mat = glm::mat4(kF / aspect, 0, 0, 0, 0, kF, 0, 0, 0, 0, 0, -1, 0, 0,
                         z_near, 0);
    return mat;
}
} // namespace ume