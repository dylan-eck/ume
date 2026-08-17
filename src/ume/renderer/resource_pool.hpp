#pragma once

#include <cstdint>
#include <vector>
#include <optional>

namespace ume {

inline constexpr uint32_t kHandleIndexBits = 20;
inline constexpr uint32_t kHandleIndexMask = (1u << kHandleIndexBits) - 1;
inline constexpr uint32_t kHandleMaxGeneration =
    (1u << (32 - kHandleIndexBits)) - 1;
template <typename T, typename HandleT> class ResourcePool {
public:
    [[nodiscard]] HandleT insert(T value) {
        uint32_t index = 0;
        if (!free_list_.empty()) {
            index = free_list_.back();
            free_list_.pop_back();
        } else {
            if (slots_.size() > kHandleIndexMask) {
                return HandleT{};
            }
            index = static_cast<uint32_t>(slots_.size());
            slots_.emplace_back();
        }

        Slot &slot = slots_[index];
        slot.value = std::move(value);
        slot.alive = true;

        return HandleT{(slot.generation << kHandleIndexBits) | index};
    }

    [[nodiscard]] T *get(HandleT handle) {
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

    [[nodiscard]] std::optional<T> remove(HandleT handle) {
        T *value = get(handle);
        if (value == nullptr) {
            return std::nullopt;
        }

        const uint32_t index = handle.id & kHandleIndexMask;
        Slot &slot = slots_[index];

        T out = std::move(slot.value);
        slot.value = T{};
        slot.alive = false;

        slot.generation =
            slot.generation > kHandleMaxGeneration ? 1 : slot.generation + 1;

        free_list_.push_back(index);
        return out;
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