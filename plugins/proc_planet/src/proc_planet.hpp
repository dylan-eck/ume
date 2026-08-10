#pragma once
#include "ume/plugin/plugin_api.h"

namespace proc_planet {
class Planet {
public:
    explicit Planet(const UmePluginApi *api);
    ~Planet();

    void update(const UmeFrameContext *frame_context);

private:
    const UmePluginApi *api_;
    double radius_ = 0;

    UmeMeshHandle mesh_ = UME_MESH_HANDLE_INVALID;
};
} // namespace proc_planet