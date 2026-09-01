#pragma once

#include "renderer_backend.hpp"

#include "ume/core/resource_pool.hpp"
#include "ume/renderer/camera.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <chrono>
#include <filesystem>

namespace ume {
class ShaderCompiler;

struct Vertex {
    glm::vec4 position;
    glm::vec4 normal;
};

struct MeshDescription {
    std::span<const float> positions;
    std::span<const float> normals;
    std::span<const uint32_t> indices;
};

class Renderer {
public:
    explicit Renderer(const Window &window);
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;

    [[nodiscard]] MeshHandle createMesh(const MeshDescription &desc);
    void destroyMesh(MeshHandle handle);

    void submit(MeshHandle handle, const glm::dvec3 &world_position,
                const glm::mat4 &local_transform);

    [[nodiscard]] PostEffectHandle
    createPostEffect(const std::filesystem::path &shader);
    bool reloadPostEffect(PostEffectHandle handle);
    void destroyPostEffect(PostEffectHandle handle);

    void submitPostEffect(PostEffectHandle, std::span<const std::byte> params);

    void setCamera(const CameraState &camera_state);
    [[nodiscard]] CameraState getCamera() const { return camera_state_; };
    [[nodiscard]] float getAspect() const { return aspect_; };

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
        glm::dvec3 world_position;
        glm::mat4 local_transform;
    };

    struct PostEffect {
        PipelineHandle pipeline;
        uint32_t params_size;
        std::filesystem::path source;
    };

    struct PostSubmission {
        PipelineHandle pipeline;
        size_t param_offset;
        size_t param_size;
    };

    uint32_t pixel_width_;
    uint32_t pixel_height_;
    float aspect_;
    std::chrono::steady_clock::time_point start_time_ =
        std::chrono::steady_clock::now();
    std::unique_ptr<RendererBackend> backend_;
    std::unique_ptr<ShaderCompiler> compiler_;

    ResourcePool<Mesh, MeshHandle> meshes_;
    std::vector<Submission> submissions_;

    ResourcePool<PostEffect, PostEffectHandle> post_effects_;
    std::vector<PostSubmission> post_submissions_;
    std::vector<std::byte> post_param_arena_;
    std::vector<PostProcessPass> post_passes_;

    CameraState camera_state_;
};
} // namespace ume