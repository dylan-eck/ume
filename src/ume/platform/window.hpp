#pragma once

#include <string>
#include <memory>

struct SDL_Window;

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

    [[nodiscard]] uint32_t getPixelWidth() const { return pixel_width_; };
    [[nodiscard]] uint32_t getPixelHeight() const { return pixel_height_; };

private:
    std::unique_ptr<SDL_Window, SDLWindowDeleter> window_;
    uint32_t pixel_width_;
    uint32_t pixel_height_;
};
} // namespace ume