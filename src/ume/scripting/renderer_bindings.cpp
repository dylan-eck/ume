#include "ume/scripting/renderer_bindings.hpp"
#include "ume/core/logger.hpp"

namespace ume {
namespace {
struct VertexArray {
    std::vector<Vertex> data;

    explicit VertexArray(size_t size) : data(size) {}
    [[nodiscard]] size_t size() const { return data.size(); }

    [[nodiscard]] Vertex &at(size_t i) {
        if (i >= data.size()) {
            throw std::out_of_range("vertex index out of range");
        }
        return data[i];
    }

    void setPosition(size_t i, float x, float y, float z) {
        at(i).position = {x, y, z, 1.0f};
    }

    void setNormal(size_t i, float x, float y, float z) {
        at(i).normal = {x, y, z, 0.0f};
    }
};

struct IndexArray {
    std::vector<uint32_t> data;

    explicit IndexArray(size_t size) : data(size) {}
    [[nodiscard]] size_t size() const { return data.size(); }

    void setTriangle(size_t tri, uint32_t a, uint32_t b, uint32_t c) {
        size_t base = tri * 3;
        if (base + 2 >= data.size()) {
            throw std::out_of_range("triangle index out of range");
        }

        data[base] = a;
        data[base + 1] = b;
        data[base + 2] = c;
    }
};
} // namespace

void bindRenderer(sol::state &lua_state, Renderer &renderer) {
    auto ume = lua_state.create_named_table("Ume");

    ume.new_usertype<MeshHandle>("MeshHandle");

    ume.new_usertype<VertexArray>(
        "VertexArray", sol::constructors<VertexArray(size_t)>(), "size",
        &VertexArray::size, "set_position", &VertexArray::setPosition,
        "set_normal", &VertexArray::setNormal);

    ume.new_usertype<IndexArray>(
        "IndexArray", sol::constructors<IndexArray(size_t)>(), "size",
        &IndexArray::size, "set_triangle", &IndexArray::setTriangle);

    ume.set_function("set_camera", [&renderer](float px, float py, float pz,
                                               float tx, float ty, float tz,
                                               float fov_y_deg) {
        renderer.setCamera({px, py, pz}, {tx, ty, tz}, glm::radians(fov_y_deg));
    });

    ume.set_function("create_mesh", [&renderer](VertexArray &v, IndexArray &i) {
        return renderer.createMesh(
            MeshDescription{.vertices = v.data, .indices = i.data});
    });

    ume.set_function("draw", [&renderer](MeshHandle handle) {
        renderer.submit(handle, glm::mat4(1.0f));
    });
}

} // namespace ume