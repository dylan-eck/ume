#pragma once

#include <string>

struct SDL_Window;

namespace ume {
struct WindowConfig {
    std::string title;
    uint32_t width{1280};
    uint32_t height{720};
};

class Window {
public:
    explicit Window(const WindowConfig &config);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool pollEvents();
    [[nodiscard]] void *getNativeHandle() const;

private:
    SDL_Window *window_{nullptr};
};
} // namespace ume