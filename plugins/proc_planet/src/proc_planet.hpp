#pragma once
#include "ume/plugin/plugin_api.h"

#include <vector>
namespace proc_planet {
class Planet {
public:
    explicit Planet(const UmePluginApi *api);
    ~Planet();

    void generate();
    void update(const UmeFrameContext *frame_context);

private:
    const UmePluginApi *api_;
    double radius_ = 0;

    std::vector<UmeMeshHandle> meshes_;
};
} // namespace proc_planet