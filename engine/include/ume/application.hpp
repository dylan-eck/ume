#pragma once

#include <memory>
#include <string>

#include "../../src/platform/window.hpp"

namespace ume {

struct ApplicationConfig {
    std::string name;
    uint32_t width;
    uint32_t height;
};

class Application {
public:
    explicit Application(const ApplicationConfig &config);
    virtual ~Application();

    void run();

private:
    Window window;
};

std::unique_ptr<Application> createApplication();
} // namespace ume