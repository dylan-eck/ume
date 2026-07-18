#pragma once

#include <memory>

namespace ume {

class WindowImpl;

class Window {
public:
    Window();
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