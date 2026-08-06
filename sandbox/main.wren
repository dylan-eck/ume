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
        __camera_distance = __planet_radius + 200
        __camera_height = 0
        __fov_y = 30

        __t = 0.7853981633974483
        __rotation_speed = 0.2


        var resolution = 8

        __meshes = [
            quad(Vector3.new(0, 0, 1), resolution, __planet_radius),
            quad(Vector3.new(0, 0, -1), resolution, __planet_radius),
            quad(Vector3.new(1, 0, 0), resolution, __planet_radius),
            quad(Vector3.new(-1, 0, 0), resolution, __planet_radius),
            quad(Vector3.new(0, 1, 0), resolution, __planet_radius),
            quad(Vector3.new(0, -1, 0), resolution, __planet_radius)
        ]

        __marker = [
            quad(Vector3.new(0, 0, 1), 8, 20),
            quad(Vector3.new(0, 0, -1), 8, 20),
            quad(Vector3.new(1, 0, 0), 8, 20),
            quad(Vector3.new(-1, 0, 0), 8, 20),
            quad(Vector3.new(0, 1, 0), 8, 20),
            quad(Vector3.new(0, -1, 0), 8, 20)
        ]

        Renderer.setCamera(__camera_distance, 0, 0, 0, 0, 0, __fov_y)
    }

    static update(delta) {
        __t = __t + __rotation_speed * delta

        var r = __camera_distance
        var cam_x = __camera_distance * __t.cos
        var cam_z = __camera_distance * __t.sin

        var marker_r = __camera_distance - 100
        var marker_x = marker_r * __t.cos
        var marker_z = marker_r * __t.sin

        Renderer.setCamera(cam_x, 0, cam_z, 0, 0, 0, __fov_y)

        for (mesh in __meshes) { Renderer.submit(mesh, 0, 0, 0) }
        for (mesh in __marker) { Renderer.submit(mesh, marker_x, 0, marker_z) }
    }

    static addVertex(positions, normals, p) {
        var n = Vector3.norm(p)

        positions.add(p.x)
        positions.add(p.y)
        positions.add(p.z)

        normals.add(n.x)
        normals.add(n.y)
        normals.add(n.z)
    }

    static quad(normal, resolution, scale) {
        var positions = []
        var normals = []
        var indices = []

        var n = normal
        var a = Vector3.new(n.y, n.z, n.x)
        var b = Vector3.cross(n, a)

        a = a * 2
        b = b * 2

        var step = 1 / resolution

        for (i in 0..(resolution - 1)) {
            for (j in 0..(resolution - 1)) {
                var t = i * step - 0.5
                var u = j * step - 0.5

                var p00 = Vector3.cubeToSphere(n + a * t + b * u)
                var p10 = Vector3.cubeToSphere(n + a * (t + step) + b * u)
                var p11 = Vector3.cubeToSphere(n + a * (t + step) + b * (u + step))
                var p01 = Vector3.cubeToSphere(n + a * t + b * (u + step))

                p00 = p00 * scale
                p10 = p10 * scale
                p11 = p11 * scale
                p01 = p01 * scale

                var s = positions.count / 3

                addVertex(positions, normals, p00)
                addVertex(positions, normals, p10)
                addVertex(positions, normals, p11)
                addVertex(positions, normals, p01)

                indices.add(s)
                indices.add(s + 1)
                indices.add(s + 2)

                indices.add(s + 2)
                indices.add(s + 3)
                indices.add(s)
            }
        }

        return Renderer.createMesh(positions, normals, indices)
    }
}