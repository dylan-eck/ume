#pragma once

#include <string>

struct SDL_Window;

namespace ume {

struct Project {
    std::string name;
    std::string main_script;
};

struct ApplicationConfig {
    std::string working_dir;
};

class Application {
public:
    explicit Application(ApplicationConfig &config);
    ~Application();

    void run();

private:
    SDL_Window *window_{nullptr};
};
} // namespace ume