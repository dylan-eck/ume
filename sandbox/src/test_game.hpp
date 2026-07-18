#pragma once

#include <ume/game.hpp>

class TestGame : public ume::Game {
public:
    void initialize();
    void update();
    void shutdown();
};