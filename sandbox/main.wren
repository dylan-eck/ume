class Renderer {
    foreign static createMesh(positions, normals, indices)

    foreign static submit(mesh, px, py, pz)

    foreign static setCamera(px, py, pz, tx, ty, tz, fovYDegrees)
}

class Vector3 {
    construct new(x, y, z) {
        _x = x
        _y = y
        _z = z
    }

    +(other) {
        return Vector3.new(_x + other.x, _y + other.y, _z + other.z)
    }

    -(other) {
        return Vector3.new(_x - other.x, _y - other.y, _z - other.z)
    }

    /(other) {
        if (other is Vector3) {
            return Vector3.new(_x / other.x, _y / other.y, _z / other.z)
        } else {
            return Vector3.new(_x / other, _y / other, _z / other)
        }
    }

    *(other) {
        if (other is Vector3) {
            return Vector3.new(_x * other.x, _y * other.y, _z * other.z)
        } else {
            return Vector3.new(_x * other, _y * other, _z * other)
        }
    }

    static dot(a, b) {
        return a.x * b.x + a.y * b.y + a.z * b.z
    }

    static norm(v) {
        return v / v.length
    }

    static cross(a, b) {
        return Vector3.new(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        )
    }

    static cubeToSphere(p) {
        return Vector3.new(
            p.x * (1 - p.y * p.y / 2 - p.z * p.z / 2 + p.y * p.y * p.z * p.z / 3).sqrt,
            p.y * (1 - p.z * p.z / 2 - p.x * p.x / 2 + p.z * p.z * p.x * p.x / 3).sqrt,
            p.z * (1 - p.x * p.x / 2 - p.y * p.y / 2 + p.x * p.x * p.y * p.y / 3).sqrt
        )
    }

    x { _x }
    x=(value) { _x = value }
    y { _y }
    y=(value) { _y = value }
    z { _z }
    z=(value) { _z = value }

    length { Vector3.dot(this, this).sqrt }
    toString { "(%(_x), %(_y), %(_z))" }
}

class Sandbox {
    static init() {
        __planet_radius = 7000000
        // __camera_distance = __planet_radius + 200
        __camera_distance = 4
        __camera_height = 0
        __fov_y = 30

        __t = 0
        __rotation_speed = 1.0


        var resolution = 8

        Renderer.setCamera(0, 0, __camera_distance, 0, 0, 0, __fov_y)
    }

    static update(delta) {
        __t = __t + __rotation_speed * delta

        var r = __camera_distance
        var cam_x = __camera_distance * __t.cos
        var cam_z = __camera_distance * __t.sin

        Renderer.setCamera(cam_x, 0, cam_z, 0, 0, 0, __fov_y)
    }
}