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

class Input {
    foreign static keyCode(name)

    foreign static keyDown_(code)
    foreign static keyPressed_(code)
    foreign static keyReleased_(code)

    static resolve_(key) {
        if (key is Num) return key

        if (__codes == null) __codes = {}

        var code = __codes[key]
        if (code == null) {
            code = keyCode(key)
            __codes[key] = code
        }

        return code
    }

    static keyDown(key) { keyDown_(resolve_(key))}
    static keyPressed(key) { keyPressed_(resolve_(key))}
    static keyReleased(key) { keyReleased_(resolve_(key))}
}

class Renderer {
    foreign static createMesh(positions, normals, indices)
    foreign static submit(mesh, px, py, pz)
    foreign static setCamera(px, py, pz, dx, dy, dz, fovYDegrees)
    foreign static rotateCameraLocal(yaw, pitch, roll)
    foreign static translateCameraLocal(x, y, z)
}

