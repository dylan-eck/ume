#include <memory>

#include <ume/application.hpp>

std::unique_ptr<ume::Application> ume::createApplication() {
    ume::ApplicationConfig config{
        .name = "Test App", .width = 1280, .height = 720};

    return std::make_unique<ume::Application>(config);
}