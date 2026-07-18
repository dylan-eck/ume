#pragma once

#include <ume/game.hpp>
#include <ume/window.hpp>

namespace ume {

struct ApplicationConfig {
    WindowConfig window;
};

class Application {
public:
    template <typename TGame>
    static int run(const ApplicationConfig &config = {}) {
        static_assert(std::is_base_of_v<ume::Game, TGame>,
                      "TGame must derive from ume::Game");

        TGame game;
        game.initialize();

        Window window(config.window);

        while (!window.shouldClose()) {
            window.pollEvents();

            game.update();
        }

        game.shutdown();

        return 0;
    }
};
} // namespace ume