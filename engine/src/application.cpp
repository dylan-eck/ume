#include <ume/application.hpp>

namespace ume {
Application::Application(const ApplicationConfig &config) {

    window_ = std::make_unique<Window>(WindowConfig{
        .title = config.name, .width = config.width, .height = config.height});
}

void Application::run() {
    onStart();

    while (!window_->shouldClose()) {
        if (!window_->pollEvents()) {
            break;
        }

        onUpdate(0.0f);

        // rendering stuff here
    }

    onShutdown();
}

} // namespace ume