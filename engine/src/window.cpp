#include <SDL3/SDL.h>

#include "window.hpp"

namespace ume {

Window::Window(const WindowConfig &config) {
    SDL_Init(SDL_INIT_VIDEO);

    window_ =
        SDL_CreateWindow(config.title.c_str(), config.width, config.height, 0);
}

Window::~Window() {
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }

    SDL_Quit();
}

Window::Window(Window &&) noexcept = default;

Window &Window::operator=(Window &&) noexcept = default;

bool Window::pollEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            close_requested_ = true;
        }
    }

    return !close_requested_;
}

bool Window::shouldClose() const { return close_requested_; }
} // namespace ume