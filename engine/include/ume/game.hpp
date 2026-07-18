#pragma once

namespace ume {
class Game {
public:
    virtual void initialize() = 0;
    virtual void update() = 0;
    virtual void shutdown() = 0;
};
} // namespace ume