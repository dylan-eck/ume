import "ume" for Engine, Renderer, Input

class Application {
    static init() {
        __camera_x = 0
        __camera_y = 0
        __camera_z = 40000000

        __fov_y = 45

        __camera_move_speed = 20000000
        __camera_rotate_speed = 1.0

        Renderer.setCamera(__camera_x, __camera_y, __camera_z, 0, 0, -1, __fov_y)

        __planet = Engine.createObject(
            "proc_planet.Planet",
            {"radius": 7000000, "x": 0, "y": 0, "z": 0})
    }

    static update(delta) {
        var dPosition = __camera_move_speed * delta
        if (Input.keyDown("A")) {
            Renderer.translateCameraLocal(-dPosition, 0, 0)
        }
        if (Input.keyDown("D")) {
            Renderer.translateCameraLocal(dPosition, 0, 0)
        }
        if (Input.keyDown("W")) {
            Renderer.translateCameraLocal(0, 0, -dPosition)
        }
        if (Input.keyDown("S")) {
            Renderer.translateCameraLocal(0, 0, dPosition)
        }
        if (Input.keyDown("LEFT SHIFT")) {
            Renderer.translateCameraLocal(0, -dPosition, 0)
        }
        if (Input.keyDown("SPACE")) {
            Renderer.translateCameraLocal(0, dPosition, 0)
        }

        var dTheta = __camera_rotate_speed * delta
        if (Input.keyDown("left")) {
            Renderer.rotateCameraLocal(dTheta, 0, 0)
        }
        if (Input.keyDown("right")) {
            Renderer.rotateCameraLocal(-dTheta, 0, 0)
        }
        if (Input.keyDown("up")) {
            Renderer.rotateCameraLocal(0, dTheta, 0)
        }
        if (Input.keyDown("down")) {
            Renderer.rotateCameraLocal(0, -dTheta, 0)
        }
        if (Input.keyDown("q")) {
            Renderer.rotateCameraLocal(0, 0, dTheta)
        }
        if (Input.keyDown("e")) {
            Renderer.rotateCameraLocal(0, 0, -dTheta)
        }
    }
}
