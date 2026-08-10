#include "proc_planet.hpp"

#include <iostream>

namespace proc_planet {
Planet::Planet() { std::cout << "Hello from Planet constructor\n"; }

void Planet::update() {
    // std::cout << "Hello from planet update function\n";
}
} // namespace proc_planet