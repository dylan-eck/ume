#pragma once

#include <string>
#include <memory>

struct WrenVM;
struct WrenHandle;
namespace ume {

class Renderer;
class PluginHost;

struct WrenVMDeleter {
    void operator()(WrenVM *vm) const;
};

using WrenVMPtr = std::unique_ptr<WrenVM, WrenVMDeleter>;

// TODO: could this hidden in the class?
struct ScriptContext {
    Renderer *renderer;
    PluginHost *plugin_host;
};

class ScriptEngine {
public:
    ScriptEngine(Renderer &renderer, PluginHost &plugin_host,
                 const std::string &main_script_path);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine &) = delete;
    ScriptEngine &operator=(const ScriptEngine &) = delete;

    ScriptEngine(ScriptEngine &&) = delete;
    ScriptEngine &operator=(ScriptEngine &&) = delete;

    void init();
    void update(float delta);

private:
    ScriptContext context_;

    WrenVMPtr wren_vm_;
    WrenHandle *main_class_;
    WrenHandle *main_init_;
    WrenHandle *main_update_;

    bool main_script_failed_ = false;
};
} // namespace ume