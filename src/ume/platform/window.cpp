#include "window.hpp"
#include "ume/core/logger.hpp"
#include "ume/core/error.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace ume {

#if defined UME_RENDER_BACKEND_METAL
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
#elif defined UME_RENDER_BACKEND_VULKAN
VkSurfaceKHR Window::createVulkanSurface(VkInstance instance) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window_.get(), instance, nullptr, &surface)) {
        const char *error = SDL_GetError();

        throw Error(logger::Category::Renderer,
                    "failed to create vulkan surface: {}", error);
    }

    return surface;
}
#endif

namespace {
SDL_Window *createSDLWindow(const WindowConfig &config) {
    SDL_Init(SDL_INIT_VIDEO);

    Uint32 flags = 0;
#if defined(UME_RENDER_BACKEND_METAL)
    flags |= SDL_WINDOW_METAL;
#elif defined(UME_RENDER_BACKEND_VULKAN)
    flags |= SDL_WINDOW_VULKAN;
#endif

    return SDL_CreateWindow(
        config.title.c_str(), static_cast<int>(config.width),
        static_cast<int>(config.height), flags | SDL_WINDOW_HIGH_PIXEL_DENSITY);
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

bool Window::pollEvents() {
    keys_pressed_.fill(false);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            return false;
        }

        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            if (e.key.scancode == SDL_SCANCODE_R) {
                keys_pressed_[static_cast<size_t>(Key::Reload)] = true;
            }
        }
    }

    return true;
}

void *Window::getNativeHandle() const { return window_.get(); };
} // namespace ume