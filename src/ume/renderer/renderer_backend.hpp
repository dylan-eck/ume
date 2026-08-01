#pragma once

#include <memory>
#include <span>

namespace ume {

struct BufferHandle {
    uint32_t id = 0;
    explicit operator bool() const { return id != 0; }
};

struct BufferDescription {
    size_t size;
    const void *initial_data;
};

enum Indextype : uint8_t { UInt16, UInt32 };

struct DrawCommand {
    BufferHandle vertex_buffer;
    uint32_t vertex_count = 0;
    BufferHandle index_buffer;
    uint32_t index_count = 0;
    Indextype index_type;
    std::span<const std::byte> push_constants;
};

class RendererBackend {
public:
    RendererBackend() = default;
    virtual ~RendererBackend() = default;

    RendererBackend(const RendererBackend &) = delete;
    RendererBackend &operator=(const RendererBackend &) = delete;

    RendererBackend(RendererBackend &&) = delete;
    RendererBackend &operator=(RendererBackend &&) = delete;

    virtual void beginFrame() = 0;
    virtual void draw(const DrawCommand &cmd) = 0;
    virtual void endFrame() = 0;

    virtual BufferHandle
    createBuffer(const BufferDescription &buffer_description) = 0;
    virtual void destroyBuffer(BufferHandle buffer) = 0;
};

std::unique_ptr<RendererBackend>
createRendererBackend(void *native_window_handle);
} // namespace ume