#pragma once

#include "renderer_backend.hpp"
#include "ume/renderer/resource_pool.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

namespace ume {
struct Vertex {
    glm::vec4 position;
    glm::vec4 normal;
};

struct MeshDescription {
    std::span<const Vertex> vertices;
    std::span<const uint32_t> indices;
};

class Renderer {
public:
    explicit Renderer(void *native_window_handle, uint32_t pixel_width,
                      uint32_t pixel_height);

    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;

    MeshHandle createMesh(const MeshDescription &desc);
    void destroyMesh(MeshHandle handle);

    void setCamera(const glm::vec3 &position, const glm::vec3 &target,
                   float fov_y);

    void submit(MeshHandle handle, const glm::mat4 &transform);
    void render();

private:
    struct Mesh {
        BufferHandle vertex_buffer;
        BufferHandle index_buffer;
        uint32_t index_count = 0;
        IndexType index_type = IndexType::UInt32;
    };

    struct Submission {
        Mesh mesh;
        glm::mat4 transform;
    };

    std::unique_ptr<RendererBackend> backend_;
    ResourcePool<Mesh, MeshHandle> meshes_;
    std::vector<Submission> submissions_;

    float aspect_;
    glm::mat4 view_;
    glm::mat4 projection_;
};
} // namespace ume