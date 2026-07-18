#include <ume/window.hpp>

#include <SDL3/SDL.h>

namespace ume {

class WindowImpl {
public:
    SDL_Window *window = nullptr;
    bool closeRequested = false;
};

Window::Window() : impl(std::make_unique<WindowImpl>()) {
    SDL_Init(SDL_INIT_VIDEO);

    impl->window = SDL_CreateWindow("Ume Engine", 800, 800, 0);
}

Window::~Window() {
    if (impl->window) {
        SDL_DestroyWindow(impl->window);
    }

    SDL_Quit();
}

Window::Window(Window &&) noexcept = default;

Window &Window::operator=(Window &&) noexcept = default;

bool Window::pollEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            impl->closeRequested = true;
        }
    }

    return !impl->closeRequested;
}

bool Window::shouldClose() const { return impl->closeRequested; }
} // namespace ume