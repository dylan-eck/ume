#include "application.hpp"

#include <iostream>

#include <SDL3/SDL.h>

namespace ume {
Application::Application() {
    SDL_Init(SDL_INIT_VIDEO);

    window_ = SDL_CreateWindow("", 1280, 720, SDL_WINDOW_VULKAN);
}

Application::~Application() {
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
}

void Application::run() {
    std::cout << "Hello from ume::Application::run()\n";
    SDL_Event e;
    bool should_quit = false;

    while (!should_quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                should_quit = true;
            }
        }
    }
}

} // namespace ume