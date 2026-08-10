#pragma once

namespace proc_planet {
class Planet {
public:
    Planet();

    void update();

private:
    double radius_ = 0;
};
} // namespace proc_planet