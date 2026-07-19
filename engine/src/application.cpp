#include <ume/application.hpp>

namespace ume {
Application::Application(const ApplicationConfig &config)
    : window({.title = config.name,
              .width = config.width,
              .height = config.height}) {}

Application::~Application() {}

void Application::run() {
    while (!window.shouldClose()) {
        if (!window.pollEvents()) {
            break;
        }
    }
}

} // namespace ume