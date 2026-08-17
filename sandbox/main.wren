class Renderer {
    foreign static createMesh(positions, normals, indices)

    foreign static submit(mesh, px, py, pz)

    foreign static setCamera(px, py, pz, tx, ty, tz, fovYDegrees)
}

class Sandbox {
    static init() {
        __camera_distance = 40000000
        __fov_y = 30

        __t = 0
        __rotation_speed = 0.2

        Renderer.setCamera(0, 0, __camera_distance, 0, 0, 0, __fov_y)
    }

    static update(delta) {
        // __t = __t + __rotation_speed * delta

        // var cam_x = __camera_distance * __t.cos
        // var cam_z = __camera_distance * __t.sin

        // Renderer.setCamera(cam_x, 0, cam_z, 0, 0, 0, __fov_y)
    }
}