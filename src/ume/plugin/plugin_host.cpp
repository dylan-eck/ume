#include "plugin_host.hpp"
#include "ume/renderer/renderer.hpp"
#include "ume/core/logger.hpp"

namespace ume {

static_assert(sizeof(Vertex) == sizeof(UmeVertex));
static_assert(alignof(Vertex) == alignof(UmeVertex));
static_assert(offsetof(Vertex, normal) == offsetof(UmeVertex, normal));

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

PluginHost::~PluginHost() = default;

bool PluginHost::registerStatic(const char *name,
                                UmePluginRegisterFunction register_function) {
    if (name == nullptr || register_function == nullptr) {
        UME_LOG_ERROR(
            Plugin,
            "attempted to register plugin with null name or register function");
        return false;
    }

    UmePluginDescription description{};

    if (!register_function(&api_, &description)) {
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
                                  const double world_position[3]) noexcept {
    auto *self = static_cast<PluginHost *>(context);
    self->renderer_.submit(
        {.id = handle},
        glm::dvec3(world_position[0], world_position[1], world_position[2]),
        glm::mat4(1.0f));
}

void PluginHost::logTrampoline(void *context, UmeLogLevel log_level,
                               const char *message) noexcept {}
} // namespace ume