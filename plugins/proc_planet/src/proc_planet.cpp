#include "proc_planet.hpp"

#include <array>

namespace proc_planet {
Planet::Planet(const UmePluginApi *api) {
    api_ = api;

    const std::array<UmeVertex, 3> kVertices{{
        {.position = {0.0f, 0.5f, 0.0f, 1.0f},
         .normal = {0.0f, 0.0f, 1.0f, 0.0f}},
        {.position = {-0.5f, -0.5f, 0.0f, 1.0f},
         .normal = {0.0f, 0.0f, 1.0f, 0.0f}},
        {.position = {0.5f, -0.5f, 0.0f, 1.0f},
         .normal = {0.0f, 0.0f, 1.0f, 0.0f}},
    }};

    const std::array<uint32_t, 3> kIndices{{0, 1, 2}};

    UmeMeshDescription desc{
        .vertices = kVertices.data(),
        .vertex_count = kVertices.size(),
        .indices = kIndices.data(),
        .index_count = kIndices.size(),
    };

    mesh_ = api_->createMesh(api_->context, &desc);

    if (mesh_ == UME_MESH_HANDLE_INVALID) {
        api_->log(api_->context, UME_LOG_LEVEL_ERROR,
                  "failed to create planet mesh");
    }
}

Planet::~Planet() {
    if (mesh_ != UME_MESH_HANDLE_INVALID) {
        api_->destroyMesh(api_->context, mesh_);
    }
}

void Planet::update(const UmeFrameContext *frame_context) {
    if (mesh_ == UME_MESH_HANDLE_INVALID) {
        return;
    }

    const std::array<double, 3> kWorldPosition = {0.0, 0.0, 0.0};
    api_->submit(api_->context, mesh_, kWorldPosition.data());
}
} // namespace proc_planet