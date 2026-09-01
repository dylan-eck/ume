#include "application.hpp"
#include "ume/core/logger.hpp"
#include "ume/core/error.hpp"

#include <glaze/toml.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <wren.hpp>

#include <string>
#include <cstring>

namespace ume {

Application::Application(const ApplicationConfig &config)
    : project_(loadProject(config.working_dir)),
      window_(getWindowConfig(project_)), renderer_(window_),
      plugin_host_(renderer_),
      script_engine_(renderer_, plugin_host_, window_.input(),
                     config.working_dir + "/" + project_.main_script),
      reload_key_(keyCodeFromName("R")) {

    registerBuiltinPlugins(
        plugin_host_, std::filesystem::path(config.working_dir) / "plugins");

    script_engine_.init();

    last_frame_time_ = std::chrono::steady_clock::now();

    UME_LOG_INFO(Core, "application initialized");
}

Application::~Application() {}

void Application::run() {
    while (window_.pollEvents()) {
        if (window_.input().keyPressed(reload_key_)) {
            UME_LOG_INFO(Core, "reloading script engine and shaders");
            script_engine_.reload();
            plugin_host_.reloadShaders();
        }

        frame_index_++;
        auto now = std::chrono::steady_clock::now();
        float delta =
            std::chrono::duration<float>(now - last_frame_time_).count();
        last_frame_time_ = now;

        script_engine_.update(delta);

        const CameraState &camera_state = renderer_.getCamera();

        UmeFrameContext context{};

        context.struct_size = sizeof(UmeFrameContext);
        context.frame_index = frame_index_;
        context.camera_position[0] = camera_state.position.x;
        context.camera_position[1] = camera_state.position.y;
        context.camera_position[2] = camera_state.position.z;
        std::memcpy(context.camera_orientation,
                    glm::value_ptr(camera_state.orientation),
                    sizeof(context.camera_orientation));
        context.fov_y = camera_state.fov_y;
        context.z_near = camera_state.z_near;
        context.aspect = renderer_.getAspect();
        context.delta_time = delta;

        plugin_host_.updateObjects(context);

        renderer_.render();
    }
}

ProjectDescription Application::loadProject(const std::string &working_dir) {
    ProjectDescription project;

    glz::error_ctx err = glz::read_file_toml(
        project, working_dir + "/project.toml", std::string{});

    if (err) {
        throw Error(logger::Category::Core, "failed to parse project.toml: {}",
                    glz::format_error(err, "project.toml"));
    }

    UME_LOG_INFO(Core, "loaded project.toml");

    return project;
}

WindowConfig getWindowConfig(const ProjectDescription &project) {
    return WindowConfig{.title = project.name,
                        .width = project.window_config.width,
                        .height = project.window_config.height};
}

} // namespace ume