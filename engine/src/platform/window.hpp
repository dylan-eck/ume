#pragma once

#include <string>

namespace ume {

struct WindowConfig {
    std::string title = "Ume Engine";
    uint32_t width = 1920;
    uint32_t height = 1080;
};

class Window {
public:
    explicit Window(const WindowConfig &config = {});
    ~Window();

    Window(Window &&) noexcept;
    Window &operator=(Window &&) noexcept;

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool pollEvents();
    bool shouldClose() const;

private:
    struct SDL_Window *window;

    bool closeRequested = false;
};
} // namespace ume