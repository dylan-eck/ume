#pragma once

#include <string>

struct WrenVM;
struct WrenHandle;
namespace ume {

class Renderer;

class ScriptEngine {
public:
    ScriptEngine(Renderer &renderer, const std::string &main_script_path);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine &) = delete;
    ScriptEngine &operator=(const ScriptEngine &) = delete;

    ScriptEngine(ScriptEngine &&) = delete;
    ScriptEngine &operator=(ScriptEngine &&) = delete;

    void init();
    void update(float delta);

private:
    WrenVM *wren_vm_;
    WrenHandle *main_class_;
    WrenHandle *main_init_;
    WrenHandle *main_update_;

    bool main_script_failed_ = false;
};
} // namespace ume