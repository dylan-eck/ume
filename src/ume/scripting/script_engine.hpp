#pragma once

#include "ume/plugin/plugin_host.hpp"
#include "ume/platform/input.hpp"

#include <string>
#include <memory>
#include <vector>

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
    const Input *input;

    std::vector<ObjectHandle> objects;
    std::vector<MeshHandle> meshes;
};

class ScriptEngine {
public:
    ScriptEngine(Renderer &renderer, PluginHost &plugin_host,
                 const Input &input, std::string main_script_path);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine &) = delete;
    ScriptEngine &operator=(const ScriptEngine &) = delete;

    ScriptEngine(ScriptEngine &&) = delete;
    ScriptEngine &operator=(ScriptEngine &&) = delete;

    void init();
    void update(float delta);

    bool reload();

private:
    // TODO: use std::filesystem::path?
    std::string main_script_path_;
    ScriptContext context_;

    WrenVMPtr wren_vm_;
    WrenHandle *main_class_ = nullptr;
    WrenHandle *main_init_ = nullptr;
    WrenHandle *main_update_ = nullptr;

    bool main_script_failed_ = false;

    void destroyScriptResources();
    void releaseVM();
};
} // namespace ume