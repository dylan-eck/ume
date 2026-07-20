#pragma once

struct SDL_Window;

namespace ume {

class Application {
public:
    explicit Application();
    ~Application();

    void run();

private:
    SDL_Window *window_{nullptr};
};
} // namespace ume