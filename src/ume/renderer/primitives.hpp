#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>

namespace ume::primitives {

struct Vertex {
    glm::vec4 position;
    glm::vec4 normal;
};

static_assert(sizeof(Vertex) == 32);

inline constexpr std::array<Vertex, 24> kCubeVertices = {{
    Vertex{.position{-0.5f, -0.5f, 0.5f, 1.0f},
           .normal{0.0f, 0.0f, 1.0f, 0.0f}},
    Vertex{.position{0.5f, -0.5f, 0.5f, 1.0f}, .normal{0.0f, 0.0f, 1.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, 0.5f, 1.0f}, .normal{0.0f, 0.0f, 1.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, 0.5f, 1.0f}, .normal{0.0f, 0.0f, 1.0f, 0.0f}},

    Vertex{.position{0.5f, -0.5f, -0.5f, 1.0f},
           .normal{0.0f, 0.0f, -1.0f, 0.0f}},
    Vertex{.position{-0.5f, -0.5f, -0.5f, 1.0f},
           .normal{0.0f, 0.0f, -1.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, -0.5f, 1.0f},
           .normal{0.0f, 0.0f, -1.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, -0.5f, 1.0f},
           .normal{0.0f, 0.0f, -1.0f, 0.0f}},

    Vertex{.position{-0.5f, -0.5f, -0.5f, 1.0f},
           .normal{-1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, -0.5f, 0.5f, 1.0f},
           .normal{-1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, 0.5f, 1.0f},
           .normal{-1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, -0.5f, 1.0f},
           .normal{-1.0f, 0.0f, 0.0f, 0.0f}},

    Vertex{.position{0.5f, -0.5f, 0.5f, 1.0f}, .normal{1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, -0.5f, -0.5f, 1.0f},
           .normal{1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, -0.5f, 1.0f}, .normal{1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, 0.5f, 1.0f}, .normal{1.0f, 0.0f, 0.0f, 0.0f}},

    Vertex{.position{-0.5f, 0.5f, 0.5f, 1.0f}, .normal{0.0f, 1.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, 0.5f, 1.0f}, .normal{0.0f, 1.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, -0.5f, 1.0f}, .normal{0.0f, 1.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, -0.5f, 1.0f},
           .normal{0.0f, 1.0f, 0.0f, 0.0f}},

    Vertex{.position{-0.5f, -0.5f, -0.5f, 1.0f},
           .normal{0.0f, -1.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, -0.5f, -0.5f, 1.0f},
           .normal{0.0f, -1.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, -0.5f, 0.5f, 1.0f},
           .normal{0.0f, -1.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, -0.5f, 0.5f, 1.0f},
           .normal{0.0f, -1.0f, 0.0f, 0.0f}},
}};

inline constexpr std::array<uint32_t, 36> kCubeIndices = {{
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
}};

} // namespace ume::primitives