#pragma once

#include <string>
#include <memory>

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
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace ume