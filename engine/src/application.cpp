#include <ume/application.hpp>

namespace ume {
Application::Application(const ApplicationConfig &config)
    : window_({.title = config.name,
               .width = config.width,
               .height = config.height}) {}

Application::~Application() {}

void Application::run() {
    while (!window_.shouldClose()) {
        if (!window_.pollEvents()) {
            break;
        }
    }
}

} // namespace ume