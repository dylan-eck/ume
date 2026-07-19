#include <SDL3/SDL.h>

#include "./window.hpp"

namespace ume {

Window::Window(const WindowConfig &config) {
    SDL_Init(SDL_INIT_VIDEO);

    window =
        SDL_CreateWindow(config.title.c_str(), config.width, config.height, 0);
}

Window::~Window() {
    if (window) {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();
}

Window::Window(Window &&) noexcept = default;

Window &Window::operator=(Window &&) noexcept = default;

bool Window::pollEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            closeRequested = true;
        }
    }

    return !closeRequested;
}

bool Window::shouldClose() const { return closeRequested; }
} // namespace ume