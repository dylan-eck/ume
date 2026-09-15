import "ume" for Engine, Window, Input, Renderer

class Application {
    static init() {
        __camera_x = 0
        __camera_y = 0
        __camera_z = 30000000

        __fov_y = 45

        __camera_move_speed = 2800000.0
        __camera_rotate_speed = 1.0

        Renderer.setCamera(__camera_x, __camera_y, __camera_z, 0, 0, -1, __fov_y)

        // __terrain = Engine.createObject("terrain.Terrain")
        __planet = Engine.createObject("proc_planet.Planet", {"radius": 7000000})

        __title_update_period = 1.0
        __title_timer = 0.0
        __title_frame_count = 0
    }

    static updateTitle(delta) {
        __title_timer = __title_timer + delta
        __title_frame_count = __title_frame_count + 1
        if (__title_timer >= __title_update_period) {
            var fps = __title_frame_count / __title_timer
            Window.setTitle("Test Application - FPS: %(fps.round)")
            __title_timer = 0.0
            __title_frame_count = 0
        }
    }

    static update(delta) {


        updateTitle(delta)

        var dPosition = __camera_move_speed * delta
        if (Input.keyDown("a")) {
            Renderer.translateCameraLocal(-dPosition, 0, 0)
        }
        if (Input.keyDown("d")) {
            Renderer.translateCameraLocal(dPosition, 0, 0)
        }
        if (Input.keyDown("w")) {
            Renderer.translateCameraLocal(0, 0, -dPosition)
        }
        if (Input.keyDown("s")) {
            Renderer.translateCameraLocal(0, 0, dPosition)
        }
        if (Input.keyDown("left shift")) {
            Renderer.translateCameraLocal(0, -dPosition, 0)
        }
        if (Input.keyDown("space")) {
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
