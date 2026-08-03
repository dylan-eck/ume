#include "application.hpp"
#include "ume/core/logger.hpp"

#include <glaze/toml.hpp>
#include <glm/glm.hpp>

#include <iostream>
#include <string>

namespace ume {

inline constexpr std::array<Vertex, 24> kCubeVertices = {{
    Vertex{.position{-0.5f, -0.5f, 0.5f, 1.0f},
           .normal{0.0f, 0.0f, 1.0f, 0.0f}},
    Vertex{.position{0.5f, -0.5f, 0.5f, 1.0f}, .normal{0.0f, 0.0f, 1.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, 0.5f, 1.0f}, .normal{0.0f, 0.0f, 1.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, 0.5f, 1.0f}, .normal{0.0f, 0.0f, 1.0f, 0.0f}},

    Vertex{.position{0.5f, -0.5f, -0.5f, 1.0f},
           .normal{0.0f, 0.0f, -1.0f, 0.0f}},
    Vertex{.position{-0.5f, -0.5f, -0.5f, 1.0f},
           .normal{0.0f, 0.0f, -1.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, -0.5f, 1.0f},
           .normal{0.0f, 0.0f, -1.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, -0.5f, 1.0f},
           .normal{0.0f, 0.0f, -1.0f, 0.0f}},

    Vertex{.position{-0.5f, -0.5f, -0.5f, 1.0f},
           .normal{-1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, -0.5f, 0.5f, 1.0f},
           .normal{-1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, 0.5f, 1.0f},
           .normal{-1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, -0.5f, 1.0f},
           .normal{-1.0f, 0.0f, 0.0f, 0.0f}},

    Vertex{.position{0.5f, -0.5f, 0.5f, 1.0f}, .normal{1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, -0.5f, -0.5f, 1.0f},
           .normal{1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, -0.5f, 1.0f}, .normal{1.0f, 0.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, 0.5f, 1.0f}, .normal{1.0f, 0.0f, 0.0f, 0.0f}},

    Vertex{.position{-0.5f, 0.5f, 0.5f, 1.0f}, .normal{0.0f, 1.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, 0.5f, 1.0f}, .normal{0.0f, 1.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, 0.5f, -0.5f, 1.0f}, .normal{0.0f, 1.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, 0.5f, -0.5f, 1.0f},
           .normal{0.0f, 1.0f, 0.0f, 0.0f}},

    Vertex{.position{-0.5f, -0.5f, -0.5f, 1.0f},
           .normal{0.0f, -1.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, -0.5f, -0.5f, 1.0f},
           .normal{0.0f, -1.0f, 0.0f, 0.0f}},
    Vertex{.position{0.5f, -0.5f, 0.5f, 1.0f},
           .normal{0.0f, -1.0f, 0.0f, 0.0f}},
    Vertex{.position{-0.5f, -0.5f, 0.5f, 1.0f},
           .normal{0.0f, -1.0f, 0.0f, 0.0f}},
}};

inline constexpr std::array<uint32_t, 36> kCubeIndices = {{
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
}};

Application::Application(const ApplicationConfig &config)
    : project_(loadProject(config.working_dir)),
      window_(getWindowConfig(project_)),
      renderer_(window_.getNativeHandle(), project_.window_config.width,
                project_.window_config.height) {

    std::cout << "lua test:\n";
    lua_state_.open_libraries(sol::lib::base);

    lua_state_.script_file(config.working_dir + "/" + project_.main_script);
    sol::table main = lua_state_["main"];
    init_ = main["init"];
    init_();
    update_ = main["update"];

    cube_mesh_ = renderer_.createMesh({
        .vertices = kCubeVertices,
        .indices = kCubeIndices,
    });

    renderer_.setCamera(glm::vec3(0.0f, 2.0f, 2.0f), glm::vec3(0.0f),
                        glm::radians(45.0f));

    UME_LOG_INFO(Core, "application initialized");
}

Application::~Application() {}

void Application::run() {
    while (window_.pollEvents()) {
        // update_(frame_count_);

        cube_transform_ =
            glm::rotate(cube_transform_, 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));

        if (cube_mesh_) {
            renderer_.submit(cube_mesh_, cube_transform_);
        }
        renderer_.render();
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

    UME_LOG_INFO(Core, "loaded project");

    return project;
}

WindowConfig getWindowConfig(const ProjectDescription &project) {
    return WindowConfig{.title = project.name,
                        .width = project.window_config.width,
                        .height = project.window_config.height};
}

} // namespace ume