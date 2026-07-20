#pragma once

#include <memory>
#include <string>

namespace ume {

// forward declarations
class Window;
class Renderer;

struct ApplicationConfig {
    std::string name;
    int width;
    int height;
};

class Application {
public:
    explicit Application(const ApplicationConfig &config);
    virtual ~Application();

    void run();

protected:
    virtual void onStart() {}
    virtual void onUpdate(float delta_time) {}
    virtual void onShutdown() {}

private:
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer> renderer_;
};

std::unique_ptr<Application> createApplication();
} // namespace ume