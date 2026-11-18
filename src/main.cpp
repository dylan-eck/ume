#include "ume/core/engine.hpp"
#include "ume/core/error.hpp"
#include "ume/core/logger.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <exception>
#include <filesystem>
#include <memory>
#include <system_error>

namespace {
std::filesystem::path resolveProjectRoot(int argc, char **argv) {
    using ume::logger::Category;

    if (argc > 2) {
        throw ume::Error(Category::Core, "usage: {} [project_dir]", argv[0]);
    }

    const std::filesystem::path requested =
        argc == 2 ? std::filesystem::path(argv[1])
                  : std::filesystem::current_path();

    std::error_code ec;
    std::filesystem::path root = std::filesystem::canonical(requested, ec);
    if (ec) {
        throw ume::Error(Category::Core, "invalid project directory '{}': {}",
                         std::filesystem::absolute(requested).string(),
                         ec.message());
    }

    if (!std::filesystem::is_directory(root, ec)) {
        throw ume::Error(Category::Core, "'{}' is not a directory",
                         root.string());
    }

    if (!std::filesystem::is_regular_file(root / "project.toml", ec)) {
        throw ume::Error(Category::Core, "no project.toml in '{}'",
                         root.string());
    }

    return root;
}
} // namespace

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    try {
        auto config =
            ume::EngineConfig{.project_root = resolveProjectRoot(argc, argv)};
        auto engine = std::make_unique<ume::Engine>(config);
        *appstate = engine.release();
    } catch (const ume::Error &err) {
        // logging macros can't be used here because category is not known at
        // compile time
        ume::logger::log(err.category(), ume::logger::Level::Error, "{}",
                         err.what());
        return SDL_APP_FAILURE;
    } catch (const std::exception &err) {
        UME_LOG_ERROR(Unknown, "{}", err.what());
        return SDL_APP_FAILURE;
    } catch (...) {
        UME_LOG_ERROR(Unknown, "unknown exception thrown");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    auto *engine = static_cast<ume::Engine *>(appstate);
    engine->tick();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    auto *engine = static_cast<ume::Engine *>(appstate);
    engine->handleEvent(*event);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    delete static_cast<ume::Engine *>(appstate);
}