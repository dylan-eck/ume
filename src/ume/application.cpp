#include "application.hpp"

#include <glaze/toml.hpp>

#include <iostream>
#include <string>

namespace ume {

Application::Application(const ApplicationConfig &config)
    : project_(loadProject(config.working_dir)),
      window_(getWindowConfig(project_)) {

    std::cout << "lua test:\n";
    lua_state_.open_libraries(sol::lib::base);

    lua_state_.script_file(config.working_dir + "/main.lua");
    sol::table main = lua_state_["main"];
    init_ = main["init"];
    init_();
    update_ = main["update"];
}

Application::~Application() {}

void Application::run() {
    // std::cout << "Hello from ume::Application::run()\n";
    while (window_.pollEvents()) {
        update_(frame_count_);

        renderer_.beginFrame();

        renderer_.endFrame();

        frame_count_++;
    }
}

ProjectDescription Application::loadProject(const std::string &working_dir) {
    ProjectDescription project;

    glz::error_ctx err = glz::read_file_toml(
        project, working_dir + "/project.toml", std::string{});

    if (err) {
        std::string err_str =
            "failed to parse toml: " + glz::format_error(err, "project.toml") +
            '\n';
        throw std::runtime_error(err_str);
    }

    std::cout << "loaded project.toml:\n";
    std::cout << "         name: " << project.name << "\n";
    std::cout << "  main_script: " << project.main_script << "\n";

    return project;
}

WindowConfig getWindowConfig(const ProjectDescription &project) {
    return WindowConfig{.title = project.name,
                        .width = project.window_config.width,
                        .height = project.window_config.height};
}

} // namespace ume