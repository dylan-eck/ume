#include "window.hpp"

#include <SDL3/SDL.h>

namespace ume {

namespace {
SDL_Window *createSDLWindow(const WindowConfig &config) {
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 flags = 0;
#if defined(UME_RENDER_BACKEND_METAL)
    flags |= SDL_WINDOW_METAL;
#elif defined(UME_RENDER_BACKEND_VULKAN)
    flags |= SDL_WINDOW_VULKAN;
#endif

    return SDL_CreateWindow(config.title.c_str(),
                            static_cast<int>(config.width),
                            static_cast<int>(config.height), flags);
}
} // namespace

void SDLWindowDeleter::operator()(SDL_Window *window) const {
    SDL_DestroyWindow(window);
}

Window::Window(const WindowConfig &config) : window_(createSDLWindow(config)) {}

bool Window::pollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            return false;
        }
    }

    return true;
}

void *Window::getNativeHandle() const { return window_.get(); };
} // namespace ume