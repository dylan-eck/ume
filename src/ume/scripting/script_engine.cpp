#include "script_engine.hpp"
#include "renderer_bindings.hpp"
#include "ume/core/logger.hpp"

#include <sol/sol.hpp>

namespace ume {
struct ScriptEngine::Impl {
    sol::state lua_state;
    sol::protected_function init;
    sol::protected_function update;
    bool update_failed = false;
};

ScriptEngine::ScriptEngine(Renderer &renderer,
                           const std::string &main_script_path)
    : impl_(std::make_unique<Impl>()) {

    impl_->lua_state.open_libraries(sol::lib::base, sol::lib::math,
                                    sol::lib::string, sol::lib::table);

    bindRenderer(impl_->lua_state, renderer);

    auto result = impl_->lua_state.safe_script_file(main_script_path,
                                                    sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        throw std::runtime_error(std::string("failed to load script: ") +
                                 err.what());
    }

    sol::optional<sol::table> main = impl_->lua_state["Main"];
    if (!main) {
        throw std::runtime_error(
            "script does not define a global 'Main' table");
    }

    impl_->init = (*main)["init"];
    impl_->update = (*main)["update"];
}

ScriptEngine::~ScriptEngine() = default;

void ScriptEngine::init() {
    if (!impl_->init.valid()) {
        return;
    }
    auto r = impl_->init();
    if (!r.valid()) {
        sol::error err = r;
        UME_LOG_ERROR(Core, "Main.init failed: {}", err.what());
    }
}

void ScriptEngine::update(float delta) {
    if (!impl_->update.valid() || impl_->update_failed) {
        return;
    }
    auto r = impl_->update(delta);
    if (!r.valid()) {
        sol::error err = r;
        UME_LOG_ERROR(Core, "Main.update failed {}", err.what());
        impl_->update_failed = true;
    }
}

} // namespace ume