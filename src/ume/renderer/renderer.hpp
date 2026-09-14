#pragma once

#include "renderer_backend.hpp"
#include "ume/core/resource_pool.hpp"
#include "ume/renderer/camera.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>

#include <memory>
#include <filesystem>

namespace ume {
class ShaderCompiler;
struct CompiledShader;

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

    void resize(uint32_t width, uint32_t height);

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
        glm::vec4 base_color = glm::vec4(1.0f);
    };

    struct PostEffect {
        GraphicsPipelineHandle pipeline;
        uint32_t params_size;
        std::filesystem::path source;
    };

    struct PostSubmission {
        GraphicsPipelineHandle pipeline;
        size_t param_offset;
        size_t param_size;
    };

    uint32_t pixel_width_;
    uint32_t pixel_height_;
    float aspect_;
    std::unique_ptr<RendererBackend> backend_;
    std::unique_ptr<ShaderCompiler> compiler_;

    ResourcePool<Mesh, MeshHandle> meshes_;
    std::vector<Submission> submissions_;

    ResourcePool<PostEffect, PostEffectHandle> post_effects_;
    std::vector<PostSubmission> post_submissions_;
    std::vector<std::byte> post_param_arena_;
    std::vector<PostProcessPass> post_passes_;

    CameraState camera_state_;

    ComputePipelineHandle compute_test_pipeline_;
    std::unique_ptr<CompiledShader> compute_test_shader_;
    BufferHandle input_;
    BufferHandle output_;

    static glm::vec4 debugColorFromId(uint32_t id) {
        uint32_t h = id * 0x9e3779b9u;
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;

        const float hue = static_cast<float>(h) * (1.0f / 4294967296.0f);
        return glm::vec4(glm::rgbColor(glm::vec3(hue * 360.0f, 0.75f, 0.8f)),
                         1.0f);
    }
};
} // namespace ume