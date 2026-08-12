#pragma once

#include <string>
#include <memory>

struct WrenVM;
struct WrenHandle;
namespace ume {

class Renderer;

struct WrenVMDeleter {
    void operator()(WrenVM *vm) const;
};

using WrenVMPtr = std::unique_ptr<WrenVM, WrenVMDeleter>;

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
    WrenVMPtr wren_vm_;
    WrenHandle *main_class_;
    WrenHandle *main_init_;
    WrenHandle *main_update_;

    bool main_script_failed_ = false;
};
} // namespace ume