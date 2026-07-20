#include <ume/application.hpp>

#include "window.hpp"
#include "renderer.hpp"

namespace ume {
Application::Application(const ApplicationConfig &config) {

    window_ = std::make_unique<Window>(WindowConfig{
        .title = config.name, .width = config.width, .height = config.height});

    renderer_ = std::make_unique<Renderer>(*window_);
}

Application::~Application() = default;

void Application::run() {
    onStart();

    while (!window_->shouldClose()) {
        if (!window_->pollEvents()) {
            break;
        }

        onUpdate(0.0f);

        // rendering stuff here
        renderer_->beginFrame();
        renderer_->endFrame();
    }

    onShutdown();
}

} // namespace ume