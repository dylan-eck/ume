#pragma once

#include "ume/plugin/plugin_api.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace ume {
class Renderer;

class PluginHost {
public:
    struct Object {
        const UmeObjectType *type;
        void *instance;
    };

    explicit PluginHost(Renderer &renderer);
    ~PluginHost();

    PluginHost(const PluginHost &) = delete;
    PluginHost &operator=(const PluginHost &) = delete;

    PluginHost(PluginHost &&) = delete;
    PluginHost &operator=(PluginHost &&) = delete;

    [[nodiscard]] bool
    registerStatic(const char *name,
                   UmePluginRegisterFunction register_function);

    [[nodiscard]] Object *createObject(const char *type_name,
                                       const UmeParams *params);
    void destroyObject(Object *object);
    void updateObjects(const UmeFrameContext &frame_context);

    static const UmeParams kDefaultParams;

private:
    Renderer &renderer_;
    UmePluginApi api_{};
    std::vector<UmePluginDescription> plugins_;
    std::unordered_map<std::string, UmeObjectType> types_;
    std::vector<std::unique_ptr<Object>> live_;

    static void
    registerObjectTypeTrampoline(void *context,
                                 const UmeObjectType *type) noexcept;
    static UmeMeshHandle
    createMeshTrampoline(void *context,
                         const UmeMeshDescription *description) noexcept;
    static void destroyMeshTrampoline(void *context,
                                      UmeMeshHandle handle) noexcept;
    static void submitTrampoline(void *context, UmeMeshHandle handle,
                                 const double *world_position) noexcept;
    static void logTrampoline(void *context, UmeLogLevel log_level,
                              const char *message) noexcept;
};

void registerBuiltinPlugins(PluginHost &host);
} // namespace ume