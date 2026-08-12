#pragma once

#include <string>
#include <memory>

struct SDL_Window;

#ifdef UME_RENDER_BACKEND_METAL
// NOLINTBEGIN(readability-identifier-naming)
namespace CA {
class MetalLayer;
}
// NOLINTEND(readability-identifier-naming)

namespace ume {
class MetalSurface {
public:
    MetalSurface() = default;

    [[nodiscard]] CA::MetalLayer *getLayer() const;
    [[nodiscard]] bool valid() const { return view_ != nullptr; }

private:
    friend class Window;
    explicit MetalSurface(void *view) : view_(view) {}

    struct ViewDeleter {
        void operator()(void *view) const;
    };

    std::unique_ptr<void, ViewDeleter> view_;
};
} // namespace ume
#endif

namespace ume {
struct WindowConfig {
    std::string title;
    uint32_t width{1280};
    uint32_t height{720};
};

struct SDLWindowDeleter {
    void operator()(SDL_Window *window) const;
};

class Window {
public:
    explicit Window(const WindowConfig &config);

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    Window(Window &&) = delete;
    Window &operator=(Window &&) = delete;

    bool pollEvents();
    [[nodiscard]] void *getNativeHandle() const;

#ifdef UME_RENDER_BACKEND_METAL
    [[nodiscard]] MetalSurface createMetalSurface() const;
#endif

    [[nodiscard]] uint32_t getPixelWidth() const { return pixel_width_; };
    [[nodiscard]] uint32_t getPixelHeight() const { return pixel_height_; };

private:
    std::unique_ptr<SDL_Window, SDLWindowDeleter> window_;
    uint32_t pixel_width_ = 0;
    uint32_t pixel_height_ = 0;
};
} // namespace ume