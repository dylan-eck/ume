#pragma once

#include "../platform/window.hpp"
#include "../renderer/renderer.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <string>

struct SDL_Window;

namespace ume {

struct ProjectDescription {
    std::string name;
    std::string main_script;
    WindowConfig window_config;
};

struct ApplicationConfig {
    std::string working_dir;
};

class Application {
public:
    explicit Application(const ApplicationConfig &config);
    ~Application();

    void run();

private:
    ProjectDescription project_;
    Window window_;
    Renderer renderer_;

    int frame_count_ = 0;

    sol::state lua_state_;
    sol::function init_;
    sol::function update_;

    static ProjectDescription loadProject(const std::string &working_dir);
};

WindowConfig getWindowConfig(const ProjectDescription &project);
} // namespace ume