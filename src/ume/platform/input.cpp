#include "input.hpp"

#include "SDL3/SDL_keyboard.h"

namespace ume {
static_assert(kKeyCodeCount >= static_cast<size_t>(SDL_SCANCODE_COUNT));

KeyCode keyCodeFromName(const char *name) {
    return static_cast<KeyCode>(SDL_GetScancodeFromName(name));
}
} // namespace ume