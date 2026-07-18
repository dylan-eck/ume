#include <iostream>

#include <ume/application.hpp>

namespace ume {

Application::Application() {}

void Application::run() {
    std::cout << "Hello from ume::Application::run()!" << std::endl;
}
} // namespace ume