#include <ume/application.hpp>

#include "test_game.hpp"

int main() {
    ume::ApplicationConfig config{
        .window{.title = "Test Game", .width = 1080, .height = 720}};

    return ume::Application::run<TestGame>(config);
}