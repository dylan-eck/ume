#pragma once

#include <memory>
#include <string>

#include "../../src/platform/window.hpp"

namespace ume {

struct ApplicationConfig {
    std::string name;
    int width;
    int height;
};

class Application {
public:
    explicit Application(const ApplicationConfig &config);
    virtual ~Application() = default;

    void run();

protected:
    virtual void onStart() {}
    virtual void onUpdate(float delta_time) {}
    virtual void onShutdown() {}

    std::unique_ptr<Window> window_;
};

std::unique_ptr<Application> createApplication();
} // namespace ume