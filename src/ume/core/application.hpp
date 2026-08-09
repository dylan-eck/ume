#pragma once

#include "ume/platform/window.hpp"
#include "ume/renderer/renderer.hpp"
#include "ume/plugin/plugin_host.hpp"
#include "ume/scripting/script_engine.hpp"

#include <string>
#include <memory>
#include <chrono>

struct SDL_Window;
struct WrenVM;

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

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;

    void run();

private:
    ProjectDescription project_;
    Window window_;
    Renderer renderer_;
    PluginHost plugin_host_;

    std::unique_ptr<ScriptEngine> script_engine_ = nullptr;
    WrenVM *vm_ = nullptr;

    std::chrono::steady_clock::time_point last_frame_time_;

    static ProjectDescription loadProject(const std::string &working_dir);
};

WindowConfig getWindowConfig(const ProjectDescription &project);
} // namespace ume