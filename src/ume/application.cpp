#include "application.hpp"

#include <SDL3/SDL.h>
#include <glaze/toml.hpp>

#include <iostream>
#include <string>

namespace ume {

Application::Application(ApplicationConfig &config) {
    Project project{};

    glz::error_ctx err = glz::read_file_toml(
        project, config.working_dir + "/project.toml", std::string{});

    if (err) {
        std::string err_str =
            "failed to parse toml: " + glz::format_error(err, "project.toml") +
            '\n';
        throw std::runtime_error(err_str);
    }

    std::cout << "loaded project.toml:\n";
    std::cout << "         name: " << project.name << "\n";
    std::cout << "  main_script: " << project.main_script << "\n";

    std::cout << "lua test:\n";
    lua_state_.open_libraries(sol::lib::base);

    lua_state_.script_file(config.working_dir + "/main.lua");
    sol::table main = lua_state_["main"];
    init_ = main["init"];
    init_();
    update_ = main["update"];

    SDL_Init(SDL_INIT_VIDEO);
    window_ =
        SDL_CreateWindow(project.name.c_str(), 1280, 720, SDL_WINDOW_VULKAN);
}

Application::~Application() {
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
}

void Application::run() {
    std::cout << "Hello from ume::Application::run()\n";
    SDL_Event e;
    bool should_quit = false;

    while (!should_quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                should_quit = true;
            }

            // update_();

            renderer_.beginFrame();

            renderer_.endFrame();
        }
    }
}

} // namespace ume