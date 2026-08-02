#pragma once

#include "renderer_backend.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

namespace ume {

class Renderer {
public:
    explicit Renderer(void *native_window_handle);

    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;

    void beginFrame();
    void endFrame();

private:
    std::unique_ptr<RendererBackend> backend_;

    glm::mat4 model_matrix_;
    BufferHandle vertex_buffer_;
    BufferHandle index_buffer_;
};
} // namespace ume