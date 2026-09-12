#include "ume/core/engine.hpp"
#include "ume/core/error.hpp"
#include "ume/core/logger.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <filesystem>
#include <memory>

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    try {
        std::filesystem::path working_dir = std::filesystem::current_path();
        auto config = ume::EngineConfig{.working_dir = working_dir.string()};
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
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    auto *engine = static_cast<ume::Engine *>(appstate);
    engine->handleEvent(*event);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    delete static_cast<ume::Engine *>(appstate);
}