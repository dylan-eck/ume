#include <memory>
#include <ume/application.hpp>

class TestApplication : public ume::Application {
public:
    TestApplication()
        : Application({
              .name = "Test App",
              .width = 1280,
              .height = 720,
          }) {}

protected:
    void onStart() override {}

    void onUpdate(float delta_time) override {}

    void onShutdown() override {}
};

std::unique_ptr<ume::Application> ume::createApplication() {
    return std::make_unique<TestApplication>();
}