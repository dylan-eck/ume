#include "plugin_host.hpp"
#include "ume/renderer/renderer.hpp"
#include "ume/core/logger.hpp"

#include <algorithm>
#include <ranges>

namespace ume {

static_assert(sizeof(Vertex) == sizeof(UmeVertex));
static_assert(alignof(Vertex) == alignof(UmeVertex));
static_assert(offsetof(Vertex, normal) == offsetof(UmeVertex, normal));

namespace {
double alwaysFallback(void *impl, const char *key, double fallback) {
    return fallback;
}
} // namespace

const UmeParams PluginHost::kDefaultParams{nullptr, alwaysFallback};

PluginHost::PluginHost(Renderer &renderer) : renderer_(renderer) {
    api_.abi_version = UME_PLUGIN_ABI_VERSION;
    api_.struct_size = sizeof(UmePluginApi);
    api_.context = this;
    api_.registerObjectType = &registerObjectTypeTrampoline;
    api_.createMesh = &createMeshTrampoline;
    api_.destroyMesh = &destroyMeshTrampoline;
    api_.submit = &submitTrampoline;
    api_.log = &logTrampoline;
}

PluginHost::~PluginHost() {
    for (auto &object : live_ | std::views::reverse) {
        if (object->type->destroy != nullptr) {
            object->type->destroy(object->type->user_data, object->instance);
        }
    }
    live_.clear();
    types_.clear();

    for (auto &plugin : plugins_ | std::views::reverse) {
        if (plugin.shutdown != nullptr) {
            plugin.shutdown(plugin.state);
        }
    }
    plugins_.clear();
};

bool PluginHost::registerStatic(const char *name,
                                UmePluginRegisterFunction register_function) {
    if (name == nullptr || register_function == nullptr) {
        UME_LOG_ERROR(
            Plugin,
            "attempted to register plugin with null name or register function");
        return false;
    }

    UmePluginDescription description{};

    if (register_function(&api_, &description) == UME_FALSE) {
        UME_LOG_ERROR(Plugin, "plugin '{}' declined to register", name);
        return false;
    }

    if (description.abi_version != UME_PLUGIN_ABI_VERSION) {
        UME_LOG_ERROR(Plugin,
                      "plugin '{}' reported abi version {} (expected {})", name,
                      description.abi_version, UME_PLUGIN_ABI_VERSION);

        if (description.shutdown != nullptr) {
            description.shutdown(description.state);
        }
        return false;
    }

    plugins_.push_back(description);
    UME_LOG_INFO(Plugin, "registered plugin '{}'", name);
    return true;
}

PluginHost::Object *PluginHost::createObject(const char *type_name,
                                             const UmeParams *params) {
    if (type_name == nullptr) {
        UME_LOG_ERROR(Plugin, "createObject called with null type name");
        return nullptr;
    }

    const auto kItr = types_.find(type_name);
    if (kItr == types_.end()) {
        UME_LOG_ERROR(Plugin, "no registered type with name '{}'", type_name);
        return nullptr;
    }

    const UmeObjectType &type = kItr->second;
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

    const auto kItr =
        std::ranges::find_if(live_, [object](const std::unique_ptr<Object> &o) {
            return o.get() == object;
        });

    if (kItr == live_.end()) {
        UME_LOG_WARN(Plugin, "attempted to destroy unknown object");
        return;
    }

    if (object->type->destroy != nullptr) {
        object->type->destroy(object->type->user_data, object->instance);
    }
    live_.erase(kItr);
}

void PluginHost::updateObjects(const UmeFrameContext &frame_context) {
    for (const auto &object : live_) {
        if (object->type->update != nullptr) {
            object->type->update(object->type->user_data, object->instance,
                                 &frame_context);
        }
    }
}

void PluginHost::registerObjectTypeTrampoline(
    void *context, const UmeObjectType *type) noexcept {
    auto *self = static_cast<PluginHost *>(context);
    self->types_.insert_or_assign(type->name, *type);
}

UmeMeshHandle PluginHost::createMeshTrampoline(
    void *context, const UmeMeshDescription *description) noexcept {
    auto *self = static_cast<PluginHost *>(context);

    std::span<const Vertex> vertices{
        reinterpret_cast<const Vertex *>(description->vertices),
        description->vertex_count};

    std::span<const uint32_t> indices{description->indices,
                                      description->index_count};

    MeshHandle handle =
        self->renderer_.createMesh({.vertices = vertices, .indices = indices});

    return UmeMeshHandle{handle.id};
}

void PluginHost::destroyMeshTrampoline(void *context,
                                       UmeMeshHandle handle) noexcept {
    auto *self = static_cast<PluginHost *>(context);
    self->renderer_.destroyMesh({.id = handle});
}

void PluginHost::submitTrampoline(void *context, UmeMeshHandle handle,
                                  const double *world_position) noexcept {
    auto *self = static_cast<PluginHost *>(context);
    self->renderer_.submit(
        {.id = handle},
        glm::dvec3(world_position[0], world_position[1], world_position[2]),
        glm::mat4(1.0f));
}

void PluginHost::logTrampoline(void *context, UmeLogLevel log_level,
                               const char *message) noexcept {}
} // namespace ume