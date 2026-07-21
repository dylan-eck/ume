#pragma once

#include "renderer/renderer.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

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
    Renderer renderer_;

    sol::state lua_state_;
    sol::function init_;
    sol::function update_;
};
} // namespace ume