#include "ume/application.hpp"

#include <filesystem>

int main() {
    std::filesystem::path working_dir = std::filesystem::current_path();

    auto config = ume::ApplicationConfig{.working_dir = working_dir.string()};
    auto app = ume::Application(config);
    app.run();
}