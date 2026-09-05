#pragma once

#include "ume/platform/window.hpp"
#include "ume/platform/input.hpp"
#include "ume/renderer/renderer.hpp"
#include "ume/plugin/plugin_host.hpp"
#include "ume/scripting/script_engine.hpp"

#include <string>
#include <chrono>

struct SDL_Window;

namespace ume {

struct ProjectDescription {
    std::string name;
    std::string main_script;
    WindowConfig window_config;
};

struct EngineConfig {
    std::string working_dir;
};

class Engine {
public:
    explicit Engine(const EngineConfig &config);
    ~Engine();

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    Engine(Engine &&) = delete;
    Engine &operator=(Engine &&) = delete;

    void run();

private:
    ProjectDescription project_;
    Window window_;
    Renderer renderer_;
    PluginHost plugin_host_;

    ScriptEngine script_engine_;

    uint64_t frame_index_ = 0;
    std::chrono::steady_clock::time_point last_frame_time_;

    KeyCode reload_key_ = kInvalidKeyCode;

    static ProjectDescription loadProject(const std::string &working_dir);
};

WindowConfig getWindowConfig(const ProjectDescription &project);
} // namespace ume