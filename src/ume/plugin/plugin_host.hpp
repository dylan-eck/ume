#pragma once

#include "ume/plugin/plugin_api.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <filesystem>

namespace ume {
class Renderer;

class PluginHost {
public:
    static constexpr uint64_t kInvalidPluginID = 0;
    struct PluginContext {
        PluginHost *host = nullptr;
        uint64_t plugin_id = kInvalidPluginID;
    };

    struct Plugin {
        uint64_t id;
        void *library = nullptr;
        std::string name;
        void *state = nullptr;
        void (*shutdown)(void *) = nullptr;
        std::unique_ptr<PluginContext> context;
        std::vector<std::string> registered_types;
        std::unordered_set<UmeMeshHandle> meshes;
    };

    struct ObjectType {
        std::string name;
        uint64_t owner;
        void *user_data = nullptr;
        void *(*create)(void *, const UmeParams *);
        void (*destroy)(void *, void *);
        void (*update)(void *, void *, const UmeFrameContext *);
    };

    struct Object {
        const ObjectType *type;
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
    bool finishRegistration(const char *name,
                            UmePluginRegisterFunction register_function);

    bool loadPlugin(const std::filesystem::path &path);
    bool unloadPlugin(uint64_t id);
    [[nodiscard]] Plugin *findPlugin(uint64_t id);

    [[nodiscard]] Object *createObject(const char *type_name,
                                       const UmeParams *params);
    void destroyObject(Object *object);
    void updateObjects(const UmeFrameContext &frame_context);

    static const UmeParams kDefaultParams;

private:
    Renderer &renderer_;
    UmePluginApi api_{};

    std::vector<Plugin> plugins_;
    uint64_t next_plugin_id_ = 1;
    uint64_t registering_id_ = kInvalidPluginID;
    std::vector<std::string> registering_types_;
    bool registration_failed_ = false;

    std::unordered_map<std::string, ObjectType> types_;
    std::vector<std::unique_ptr<Object>> live_;

    void unloadPluginAt(size_t index);

    static UME_PLUGIN_BOOL
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