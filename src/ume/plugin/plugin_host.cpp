#include "plugin_host.hpp"
#include "ume/renderer/renderer.hpp"
#include "ume/core/logger.hpp"
#include "ume/core/dynamic_library.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <ranges>
#include <cstddef>

namespace ume {

static_assert(sizeof(Vertex) == sizeof(UmeVertex));
static_assert(alignof(Vertex) == alignof(UmeVertex));
static_assert(offsetof(Vertex, normal) == offsetof(UmeVertex, normal));

namespace {
double alwaysFallback(const void *impl, const char *key,
                      double fallback) noexcept {
    return fallback;
}
} // namespace

const UmeParams PluginHost::kDefaultParams{
    .struct_size = sizeof(UmeParams),
    .impl = nullptr,
    .number = alwaysFallback,
};

PluginHost::PluginHost(Renderer &renderer) : renderer_(renderer) {
    api_.abi_version = UME_PLUGIN_ABI_VERSION;
    api_.struct_size = sizeof(UmePluginApi);
    api_.context = nullptr;
    api_.registerObjectType = &registerObjectTypeTrampoline;
    api_.createMesh = &createMeshTrampoline;
    api_.destroyMesh = &destroyMeshTrampoline;
    api_.submit = &submitTrampoline;
    api_.log = &logTrampoline;
}

PluginHost::~PluginHost() {
    while (!plugins_.empty()) {
        unloadPluginAt(plugins_.size() - 1);
    }
    if (!live_.empty() || !types_.empty()) {
        UME_LOG_ERROR(Plugin,
                      "{} orphaned object(s) and {} orphaned type(s) after all "
                      "plugins unloaded",
                      live_.size(), types_.size());
    }
};

bool PluginHost::registerStatic(const char *name,
                                UmePluginRegisterFunction register_function) {
    if (name == nullptr || register_function == nullptr) {
        UME_LOG_ERROR(
            Plugin,
            "attempted to register plugin with null name or register function");
        return false;
    }

    return finishRegistration(name, nullptr, register_function);
}

bool PluginHost::loadPlugin(const std::filesystem::path &path) {
    void *library = openLibrary(path);
    if (library == nullptr) {
        UME_LOG_ERROR(Plugin, "failed to open plugin library: {}",
                      path.string());
        return false;
    }

    auto entry = reinterpret_cast<UmePluginRegisterFunction>(
        findSymbol(library, "umePluginRegister"));
    if (entry == nullptr) {
        UME_LOG_ERROR(Plugin, "library '{}' has no umePluginRegister symbol",
                      path.string());
        closeLibrary(library);
        return false;
    }

    if (!finishRegistration(path.c_str(), library, entry)) {
        closeLibrary(library);
        return false;
    }
    return true;
}

bool PluginHost::finishRegistration(
    const char *name, void *library,
    UmePluginRegisterFunction register_function) {

    if (registering_id_ != kInvalidPluginID) {
        UME_LOG_ERROR(Plugin,
                      "cannot register plugin '{}': registration of plugin "
                      "id {} is already in progress",
                      name, registering_id_);
        return false;
    }

    // next_plugin_id is incremented unconditionally to avoid confusing log
    // messages
    registering_id_ = next_plugin_id_++;

    struct Scope {
        PluginHost *host;
        ~Scope() { host->registering_id_ = kInvalidPluginID; }
    } scope{this};

    const auto rollback = [](const UmePluginDescription &desc,
                             bool call_shutdown) {
        if (call_shutdown && desc.shutdown != nullptr) {
            desc.shutdown(desc.state);
        }
    };

    auto context = std::make_unique<PluginContext>(this, registering_id_);
    UmePluginApi api = api_;
    api.context = context.get();

    UmePluginDescription description{};
    description.struct_size = sizeof(UmePluginDescription);

    if (register_function(&api, &description) == UME_FALSE) {
        UME_LOG_ERROR(Plugin, "plugin '{}' declined to register", name);
        rollback(description, false);
        return false;
    }

    if (description.name == nullptr) {
        UME_LOG_ERROR(Plugin, "plugin '{}' did not set a name", name);
        rollback(description, true);
        return false;
    }

    if (description.abi_version != UME_PLUGIN_ABI_VERSION) {
        UME_LOG_ERROR(
            Plugin, "plugin '{}' reported abi version {} (expected {})",
            description.name, description.abi_version, UME_PLUGIN_ABI_VERSION);
        rollback(description, true);
        return false;
    }

    if (std::string(description.name).find('.') != std::string::npos) {
        UME_LOG_ERROR(Plugin, "plugin names must not contain periods");
        rollback(description, true);
        return false;
    }

    auto it = std::ranges::find(plugins_, description.name, &Plugin::name);
    if (it != plugins_.end()) {
        UME_LOG_ERROR(Plugin,
                      "currently registering plugin '{}' has the same name as "
                      "an already registered plugin",
                      description.name);
        rollback(description, true);
        return false;
    }

    plugins_.emplace_back(Plugin{
        .id = registering_id_,
        .library = library,
        .name = description.name,
        .state = description.state,
        .init = description.init,
        .shutdown = description.shutdown,
        .context = std::move(context),
    });

    Plugin *p = findPlugin(registering_id_);
    if (p->init != nullptr && p->init(p->state) == UME_FALSE) {
        UME_LOG_ERROR(Plugin, "plugin '{}' failed to initialize", p->name);
        unloadPluginAt(plugins_.size() - 1);
        return false;
    }

    UME_LOG_INFO(Plugin, "registered plugin '{}'", p->name);

    return true;
}

bool PluginHost::unloadPlugin(uint64_t id) {
    if (registering_id_ != kInvalidPluginID) {
        UME_LOG_ERROR(Plugin,
                      "cannot unload plugin while registration is in progress");
        return false;
    }

    const auto it = std::ranges::find(plugins_, id, &Plugin::id);
    if (it == plugins_.end()) {
        UME_LOG_WARN(Plugin, "attempted to unload unknown plugin (id {})", id);
        return false;
    }

    unloadPluginAt(static_cast<size_t>(it - plugins_.begin()));
    return true;
}

void PluginHost::unloadPluginAt(size_t index) {
    Plugin &plugin = plugins_[index];
    UME_LOG_INFO(Plugin, "unloading plugin at index {} (id {})", index,
                 plugin.id);

    std::vector<std::unique_ptr<Object>> to_delete;
    for (auto &object : live_) {
        if (object != nullptr && object->type->owner == plugin.id) {
            to_delete.push_back(std::move(object));
        }
    }
    std::erase(live_, nullptr);

    for (auto &object : to_delete | std::views::reverse) {
        if (object->type->destroy != nullptr) {
            object->type->destroy(object->type->user_data, object->instance);
        }
    }
    to_delete.clear();

    if (!plugin.meshes.empty()) {
        UME_LOG_WARN(Plugin, "plugin '{}' leaked {} mesh(es); reclaiming",
                     plugin.name, plugin.meshes.size());
        for (const uint32_t id : plugin.meshes) {
            renderer_.destroyMesh({.id = id});
        }
        plugin.meshes.clear();
    }

    for (const auto &type_name : plugin.registered_types) {
        types_.erase(type_name);
    }

    if (plugin.shutdown != nullptr) {
        plugin.shutdown(plugin.state);
    }

    if (plugin.library != nullptr) {
        closeLibrary(plugin.library);
    }

    plugins_.erase(plugins_.begin() + static_cast<std::ptrdiff_t>(index));
}

PluginHost::Plugin *PluginHost::findPlugin(uint64_t id) {
    const auto it = std::ranges::find(plugins_, id, &Plugin::id);
    return it != plugins_.end() ? std::to_address(it) : nullptr;
}

PluginHost::Object *PluginHost::createObject(const char *type_name,
                                             const UmeParams *params) {
    if (type_name == nullptr) {
        UME_LOG_ERROR(Plugin, "createObject called with null type name");
        return nullptr;
    }

    const auto it = types_.find(type_name);
    if (it == types_.end()) {
        UME_LOG_ERROR(Plugin, "no registered type with name '{}'", type_name);
        return nullptr;
    }

    const ObjectType &type = it->second;
    if (type.create == nullptr) {
        UME_LOG_ERROR(Plugin, "object type '{}' has no create function",
                      type_name);
        return nullptr;
    }

    void *instance = type.create(type.user_data,
                                 params != nullptr ? params : &kDefaultParams);
    if (instance == nullptr) {
        UME_LOG_ERROR(Plugin, "failed to create instance of object type '{}'",
                      type_name);
        return nullptr;
    }

    live_.push_back(
        std::make_unique<Object>(Object{.type = &type, .instance = instance}));
    return live_.back().get();
}

void PluginHost::destroyObject(Object *object) {
    if (object == nullptr) {
        return;
    }

    const auto it =
        std::ranges::find_if(live_, [object](const std::unique_ptr<Object> &o) {
            return o.get() == object;
        });

    if (it == live_.end()) {
        UME_LOG_WARN(Plugin, "attempted to destroy unknown object");
        return;
    }

    if (object->type->destroy != nullptr) {
        object->type->destroy(object->type->user_data, object->instance);
    }
    live_.erase(it);
}

void PluginHost::updateObjects(const UmeFrameContext &frame_context) {
    for (const auto &object : live_) {
        if (object->type->update != nullptr) {
            object->type->update(object->type->user_data, object->instance,
                                 &frame_context);
        }
    }
}

UME_PLUGIN_BOOL
PluginHost::registerObjectTypeTrampoline(void *context,
                                         const UmeObjectType *type) noexcept {
    if (type == nullptr) {
        UME_LOG_ERROR(
            Plugin, "attempted to register object type with null type struct");
        return UME_FALSE;
    }

    if (type->struct_size < sizeof(UmeObjectType)) {
        UME_LOG_ERROR(Plugin,
                      "object registration rejected, invalid structsize {} "
                      "(expected >= {})",
                      type->struct_size, sizeof(UmeObjectType));
        return UME_FALSE;
    }

    if (type->name == nullptr) {
        UME_LOG_ERROR(Plugin, "attempted to register object type with no name");
        return UME_FALSE;
    }

    auto *ctx = static_cast<PluginContext *>(context);
    auto *self = ctx->host;

    if (ctx->plugin_id != self->registering_id_) {
        UME_LOG_ERROR(Plugin,
                      "plugin id {} attempted to register object type '{}' "
                      "outside its own registration",
                      ctx->plugin_id, type->name);
        return UME_FALSE;
    }

    Plugin *plugin = self->findPlugin(ctx->plugin_id);

    if (plugin == nullptr) {
        UME_LOG_ERROR(Plugin,
                      "could not find owner (plugin id'{}') while registering "
                      "object type '{}'",
                      ctx->plugin_id, type->name);
    }

    ObjectType object{
        .name = plugin->name + "." + type->name,
        .owner = self->registering_id_,
        .user_data = type->user_data,
        .create = type->create,
        .destroy = type->destroy,
        .update = type->update,
    };

    const auto key = object.name;
    auto [it, inserted] = self->types_.try_emplace(key, std::move(object));

    if (!inserted) {
        UME_LOG_ERROR(Plugin,
                      "attempted to register already registered object type");
        return UME_FALSE;
    }

    plugin->registered_types.push_back(it->first);

    UME_LOG_INFO(Plugin, "registered object type '{}'", it->second.name);
    return UME_TRUE;
}

UmeMeshHandle PluginHost::createMeshTrampoline(
    void *context, const UmeMeshDescription *description) noexcept {

    auto *ctx = static_cast<PluginContext *>(context);
    auto *self = ctx->host;

    if (description == nullptr) {
        UME_LOG_ERROR(Plugin,
                      "null description passed to createMeshTrampoline");
        return UME_MESH_HANDLE_INVALID;
    }

    if (description->struct_size < sizeof(UmeMeshDescription)) {
        UME_LOG_ERROR(Plugin,
                      "wrong size struct passed to createMeshTrampoline: "
                      "received {} bytes, expected {}",
                      description->struct_size, sizeof(UmeMeshDescription));
        return UME_MESH_HANDLE_INVALID;
    }

    std::span<const Vertex> vertices{
        reinterpret_cast<const Vertex *>(description->vertices),
        description->vertex_count};

    std::span<const uint32_t> indices{description->indices,
                                      description->index_count};

    MeshHandle handle =
        self->renderer_.createMesh({.vertices = vertices, .indices = indices});

    if (handle.id != UME_MESH_HANDLE_INVALID) {
        if (Plugin *plugin = self->findPlugin(ctx->plugin_id)) {
            plugin->meshes.insert(handle.id);
        }
    }

    return UmeMeshHandle{handle.id};
}

void PluginHost::destroyMeshTrampoline(void *context,
                                       UmeMeshHandle handle) noexcept {

    auto *ctx = static_cast<PluginContext *>(context);
    auto *self = ctx->host;

    Plugin *plugin = self->findPlugin(ctx->plugin_id);
    if (plugin == nullptr || plugin->meshes.erase(handle) == 0) {
        UME_LOG_ERROR(Plugin,
                      "plugin attempted to destroy mesh {} it does not own",
                      handle);
        return;
    }

    self->renderer_.destroyMesh({.id = handle});
}

void PluginHost::submitTrampoline(void *context, UmeMeshHandle handle,
                                  const double *world_position,
                                  const float *local_transform) noexcept {

    if (handle == UME_MESH_HANDLE_INVALID) {
        UME_LOG_WARN(Plugin, "plugin submitted null mesh handle");
        return;
    }

    if (world_position == nullptr || local_transform == nullptr) {
        UME_LOG_WARN(Plugin,
                     "mesh submitted with null world position or transform");
        return;
    }

    auto *ctx = static_cast<PluginContext *>(context);
    auto *self = ctx->host;

    Plugin *plugin = self->findPlugin(ctx->plugin_id);

    if (plugin == nullptr || !plugin->meshes.contains(handle)) {
        UME_LOG_ERROR(Plugin,
                      "plugin attempted to submit mesh {} it does not own",
                      handle);
        return;
    }

    self->renderer_.submit(
        {.id = handle},
        glm::dvec3(world_position[0], world_position[1], world_position[2]),
        glm::make_mat4(local_transform));
}

void PluginHost::logTrampoline(void *context, UmeLogLevel log_level,
                               const char *message) noexcept {
    if (message == nullptr) {
        return;
    }

    auto *ctx = static_cast<PluginContext *>(context);

    switch (log_level) {
    case UME_LOG_LEVEL_INFO:
        UME_LOG_INFO(Plugin, "id {}: {}", ctx->plugin_id, message);
        break;
    case UME_LOG_LEVEL_WARN:
        UME_LOG_WARN(Plugin, "id {}: {}", ctx->plugin_id, message);
        break;
    case UME_LOG_LEVEL_ERROR:
        UME_LOG_ERROR(Plugin, "id {}: {}", ctx->plugin_id, message);
        break;
    default:
        break;
    }
}
} // namespace ume