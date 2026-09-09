#include "terrain.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>

namespace terrain {
Terrain::Terrain(const TerrainPlugin *plugin) : plugin_(plugin) { generate(); }

void Terrain::generate() {
    const UmePluginApi &api = plugin_->api;
    api.log(api.context, UME_LOG_LEVEL_INFO, "generating terrain");

    float size = 1000.0f;

    std::array<float, 12> positions{{1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f,
                                     0.0f, -1.0f, 1.0f, 0.0f, -1.0f}};

    for (auto &v : positions) {
        v *= size;
    }

    std::array<float, 12> normals{
        {0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f}};

    std::array<uint32_t, 12> indices{0, 1, 2, 0, 2, 3};

    UmeMeshDescription desc{
        .struct_size = sizeof(UmeMeshDescription),
        .vertex_count = static_cast<uint32_t>(positions.size()),
        .positions = positions.data(),
        .normals = normals.data(),
        .index_count = static_cast<uint32_t>(indices.size()),
        .indices = indices.data(),
    };

    mesh_ = api.createMesh(api.context, &desc);
}

void Terrain::update(const UmeFrameContext *frame_context) {
    const UmePluginApi &api = plugin_->api;

    glm::dvec3 world_pos(0.0);
    glm::mat4 local_transform(1.0f);

    api.submit(api.context, mesh_, glm::value_ptr(world_pos),
               glm::value_ptr(local_transform));
}
} // namespace terrain