#include "window.hpp"
#include "ume/core/logger.hpp"
#include "ume/core/error.hpp"

#include <SDL3/SDL.h>

namespace ume {

void MetalSurface::ViewDeleter::operator()(void *view) const {
    SDL_Metal_DestroyView(static_cast<SDL_MetalView>(view));
}

CA::MetalLayer *MetalSurface::getLayer() const {
    return static_cast<CA::MetalLayer *>(
        SDL_Metal_GetLayer(static_cast<SDL_MetalView>(view_.get())));
}

MetalSurface Window::createMetalSurface() const {
    SDL_MetalView view = SDL_Metal_CreateView(window_.get());

    if (view == nullptr) {
        throw Error(logger::Category::Platform,
                    "failed to create metal view: {}", SDL_GetError());
    }

    return MetalSurface(view);
}

namespace {
SDL_Window *createSDLWindow(const WindowConfig &config) {
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
    flags |= SDL_WINDOW_METAL;

    return SDL_CreateWindow(config.title.c_str(),
                            static_cast<int>(config.width),
                            static_cast<int>(config.height), flags);
}
} // namespace

void SDLWindowDeleter::operator()(SDL_Window *window) const {
    SDL_DestroyWindow(window);
}

Window::Window(const WindowConfig &config) : window_(createSDLWindow(config)) {
    int w;
    int h;

    SDL_GetWindowSizeInPixels(window_.get(), &w, &h);
    pixel_width_ = w;
    pixel_height_ = h;
}

void *Window::getNativeHandle() const { return window_.get(); }

void Window::handleEvent(const SDL_Event &event) {
    switch (event.type) {
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        pixel_width_ = event.window.data1;
        pixel_height_ = event.window.data2;
        break;

    case SDL_EVENT_WINDOW_MINIMIZED:
        minimized_ = true;
        break;

    case SDL_EVENT_WINDOW_RESTORED:
        minimized_ = false;
        break;

    default:
        break;
    }
}

void Window::setTitle(const char *title) {
    SDL_SetWindowTitle(window_.get(), title);
}
} // namespace ume
