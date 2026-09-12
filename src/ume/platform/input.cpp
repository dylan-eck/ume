#include "input.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>

namespace ume {
static_assert(kKeyCodeCount >= static_cast<size_t>(SDL_SCANCODE_COUNT));

KeyCode keyCodeFromName(const char *name) {
    return static_cast<KeyCode>(SDL_GetScancodeFromName(name));
}

void Input::handleEvent(const SDL_Event &event) {
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
        onKeyDown(static_cast<KeyCode>(event.key.scancode), event.key.repeat);
        break;

    case SDL_EVENT_KEY_UP:
        onKeyUp(static_cast<KeyCode>(event.key.scancode));
        break;

    default:
        break;
    }
}

void Input::endFrame() {
    keys_pressed_.reset();
    keys_released_.reset();
}
} // namespace ume