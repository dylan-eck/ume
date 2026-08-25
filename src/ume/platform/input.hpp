#pragma once

#include <cstddef>
#include <cstdint>
#include <bitset>

namespace ume {
using KeyCode = uint16_t;

inline constexpr KeyCode kInvalidKeyCode = 0;
inline constexpr size_t kKeyCodeCount = 512;

[[nodiscard]] KeyCode keyCodeFromName(const char *name);

class Input {
public:
    [[nodiscard]] bool keyDown(KeyCode code) const {
        return keys_down_.test(code);
    }

    [[nodiscard]] bool keyPressed(KeyCode code) const {
        return keys_pressed_.test(code);
    }

    [[nodiscard]] bool keyReleased(KeyCode code) const {
        return keys_pressed_.test(code);
    }

private:
    friend class Window;

    std::bitset<kKeyCodeCount> keys_down_;
    std::bitset<kKeyCodeCount> keys_pressed_;
    std::bitset<kKeyCodeCount> keys_released_;

    template <size_t N>
    static bool test(const std::bitset<N> &bits, size_t index) {
        return index < N && bits.test(index);
    }

    void beginFrame() {
        keys_pressed_.reset();
        keys_released_.reset();
    }

    void onKeyDown(KeyCode code, bool repeat) {
        if (code >= kKeyCodeCount) {
            return;
        }
        if (!repeat) {
            keys_pressed_.set(code);
        }
        keys_down_.set(code);
    }

    void onKeyUp(KeyCode code) {
        if (code >= kKeyCodeCount) {
            return;
        }
        keys_released_.set(code);
        keys_down_.reset(code);
    }
};
} // namespace ume