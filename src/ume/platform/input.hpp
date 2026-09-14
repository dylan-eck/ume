#pragma once

#include <cstddef>
#include <cstdint>
#include <bitset>

union SDL_Event;

namespace ume {
using KeyCode = uint16_t;

inline constexpr KeyCode kInvalidKeyCode = 0;
inline constexpr size_t kKeyCodeCount = 512;

[[nodiscard]] KeyCode keyCodeFromName(const char *name);

class Input {
public:
    void handleEvent(const SDL_Event &event);
    void endFrame();

    [[nodiscard]] bool keyDown(KeyCode code) const {
        return keys_down_.test(code);
    }

    [[nodiscard]] bool keyPressed(KeyCode code) const {
        return keys_pressed_.test(code);
    }

    [[nodiscard]] bool keyReleased(KeyCode code) const {
        return keys_released_.test(code);
    }

private:
    std::bitset<kKeyCodeCount> keys_down_;
    std::bitset<kKeyCodeCount> keys_pressed_;
    std::bitset<kKeyCodeCount> keys_released_;

    void onKeyDown(KeyCode code, bool repeat) {
        if (code >= kKeyCodeCount) return;
        if (!repeat) {
            keys_pressed_.set(code);
        }
        keys_down_.set(code);
    }

    void onKeyUp(KeyCode code) {
        if (code >= kKeyCodeCount) return;
        keys_released_.set(code);
        keys_down_.reset(code);
    }
};
} // namespace ume