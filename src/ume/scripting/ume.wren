class Renderer {
    foreign static createMesh(positions, normals, indices)
    foreign static submit(mesh, px, py, pz)
    foreign static setCamera(px, py, pz, tx, ty, tz, fovYDegrees)
}

class Engine {
    foreign static createObject_(type_name, keys, values)
    foreign static destroyObject(handle)

    static createObject(type_name) {
        return createObject_(type_name, [], [])
    }

    static createObject(type_name, params) {
        var keys = []
        var values = []

        for (entry in params) {
            keys.add(entry.key)
            values.add(entry.value)
        }

        return createObject_(type_name, keys, values)
    }
}