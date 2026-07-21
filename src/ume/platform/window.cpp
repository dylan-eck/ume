#include "window.hpp"

#include <SDL3/SDL.h>

namespace ume {

Window::Window(const WindowConfig &config) {
    SDL_Init(SDL_INIT_VIDEO);
    window_ =
        SDL_CreateWindow(config.title.c_str(), static_cast<int>(config.width),
                         static_cast<int>(config.height), SDL_WINDOW_VULKAN);
}

Window::~Window() {
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
}

bool Window::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            return false;
        }
    }

    return true;
}

void *Window::getNativeHandle() const { return window_; };
} // namespace ume