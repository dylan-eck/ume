#pragma once

#include <memory>

namespace ume {

struct WindowConfig {
    const char *title = "Ume Engine";
    uint32_t width = 1920;
    uint32_t height = 1080;
};

class WindowImpl;

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
    std::unique_ptr<WindowImpl> impl;
};
} // namespace ume