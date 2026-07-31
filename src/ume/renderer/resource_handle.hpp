#pragma once

#include <cstdint>

namespace ume {
template <typename Tag> struct ResourceHandle {
    uint32_t id = 0;
    explicit operator bool() const { return id != 0; }
    friend bool operator==(ResourceHandle, ResourceHandle) = default;
};

struct BufferTag;

using BufferHandle = ResourceHandle<BufferTag>;
} // namespace ume