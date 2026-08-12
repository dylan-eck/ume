#include "ume/core/application.hpp"
#include "ume/core/error.hpp"
#include "ume/core/logger.hpp"

#include <filesystem>

int main() {
    try {
        std::filesystem::path working_dir = std::filesystem::current_path();
        auto config =
            ume::ApplicationConfig{.working_dir = working_dir.string()};
        auto app = ume::Application(config);
        app.run();
    } catch (const ume::Error &err) {
        // logging macros can't be used here because category is not known at
        // compile time
        ume::logger::log(err.category(), ume::logger::Level::Error, "{}",
                         err.what());
        return EXIT_FAILURE;
    } catch (const std::exception &err) {
        UME_LOG_ERROR(Unknown, "{}", err.what());
        return EXIT_FAILURE;
    } catch (...) {
        UME_LOG_ERROR(Unknown, "unknown exception thrown");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}