#pragma once

#include <cstdint>
#include <vector>

inline constexpr uint32_t kHandleIndexBits = 20;
inline constexpr uint32_t kHandleIndexMask = (1u << kHandleIndexBits) - 1;
inline constexpr uint32_t kHandleMaxGeneration =
    (1u << (32 - kHandleIndexBits)) - 1;

namespace ume {
template <typename T, typename HandleT> class ResourcePool {
public:
    HandleT insert(T value) {
        uint32_t index = 0;
        if (!free_list_.empty()) {
            index = free_list_.back();
            free_list_.pop_back();
        } else {
            index = static_cast<uint32_t>(slots_.size());
            slots_.emplace_back();
        }

        Slot &slot = slots_[index];
        slot.value = std::move(value);
        slot.alive = true;

        return HandleT{(slot.generation << kHandleIndexBits) | index};
    }

    T *get(HandleT handle) {
        const uint32_t index = handle.id & kHandleIndexMask;
        const uint32_t generation = handle.id >> kHandleIndexBits;

        if (index >= slots_.size()) {
            return nullptr;
        }

        Slot &slot = slots_[index];
        if (!slot.alive || slot.generation != generation) {
            return nullptr;
        }

        return &slot.value;
    }

    T *retire(HandleT handle) {
        T *value = get(handle);
        if (value == nullptr) {
            return nullptr;
        }

        Slot &slot = slots_[handle.id & kHandleIndexMask];
        slot.alive = false;
        slot.generation =
            slot.generation >= kHandleMaxGeneration ? 1 : slot.generation + 1;

        return value;
    }

    void reclaim(HandleT handle) {
        free_list_.push_back(handle.id & kHandleIndexMask);
    }

private:
    struct Slot {
        T value{};
        uint32_t generation = 1;
        bool alive = false;
    };

    std::vector<Slot> slots_;
    std::vector<uint32_t> free_list_;
};
} // namespace ume