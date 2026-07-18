#pragma once

#include <iostream>

#include <ume/game.hpp>
#include <ume/window.hpp>

namespace ume {
class Application {
public:
    template <typename TGame> static int run() {
        std::cout << "Hello from ume::Application::run()!" << std::endl;

        static_assert(std::is_base_of_v<ume::Game, TGame>,
                      "TGame must derive from ume::Game");

        TGame game;

        Window window;

        while (!window.shouldClose()) {
            window.pollEvents();
        }

        // game.initialize();
        // game.update();
        // game.shutdown();

        return 0;
    }
};
} // namespace ume