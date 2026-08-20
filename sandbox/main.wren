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

class Sandbox {
    static init() {
        __camera_distance = 80000000
        __fov_y = 15

        Renderer.setCamera(0, 0, __camera_distance, 0, 0, 0, __fov_y)

        __planet = Engine.createObject("proc_planet.Planet", {"radius": 7000000, "x": 8000000, "y": 0, "z": 0})
        __planet = Engine.createObject("proc_planet.Planet", {"radius": 7000000, "x": -8000000, "y": 0, "z": 0})
    }

    static update(delta) {}
}