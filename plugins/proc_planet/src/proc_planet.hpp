#pragma once
#include "ume/plugin/plugin_api.h"

#include <vector>
namespace proc_planet {

class MeshRef {
public:
    MeshRef(const UmePluginApi *api, UmeMeshHandle handle)
        : api_(api), handle_(handle) {};
    ~MeshRef() {
        if (handle_ != UME_MESH_HANDLE_INVALID) {
            api_->destroyMesh(api_->context, handle_);
        }
    }

    // MeshRef must be move only to prevent accidental invalidation of the
    // underlying handle
    // e.g.from resizing an std::vector of MeshRefs
    MeshRef(const MeshRef &) = delete;
    MeshRef &operator=(const MeshRef &) = delete;

    MeshRef(MeshRef &&other) noexcept
        : api_(other.api_), handle_(other.handle_) {
        other.handle_ = UME_MESH_HANDLE_INVALID;
    }

    MeshRef &operator=(MeshRef &&other) noexcept {
        if (this != &other) {
            if (handle_ != UME_MESH_HANDLE_INVALID) {
                api_->destroyMesh(api_->context, handle_);
            }

            api_ = other.api_;
            handle_ = other.handle_;
            other.handle_ = UME_MESH_HANDLE_INVALID;
        }
        return *this;
    }

    [[nodiscard]] UmeMeshHandle getHandle() const { return handle_; }

private:
    const UmePluginApi *api_;
    UmeMeshHandle handle_;
};

class Planet {
public:
    explicit Planet(const UmePluginApi *api);
    ~Planet() = default;

    Planet(const Planet &) = delete;
    Planet &operator=(const Planet &) = delete;

    Planet(Planet &&) = delete;
    Planet &operator=(Planet &&) = delete;

    void generate();
    void update(const UmeFrameContext *frame_context);

private:
    const UmePluginApi *api_;
    double radius_ = 0;

    std::vector<MeshRef> meshes_;
};
} // namespace proc_planet