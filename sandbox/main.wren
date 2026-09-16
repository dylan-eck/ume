import "ume" for Engine, Window, Input, Renderer, Vec3

class Application {
    static init() {
        __fov_y = 45

        __camera_move_speed = 125.0
        __camera_move_speed_min = 1.0
        __camera_move_speed_max = 1e8
        __camera_move_speed_factor = 2.0
        __camera_rotate_speed = 1.0

        __planet_radius = 250
        __planet = Engine.createObject("proc_planet.Planet", {"radius": __planet_radius})

         Renderer.setCamera(0, 0, 4 * __planet_radius, 0, 0, -1, __fov_y)

        __fps_update_period = 1.0
        __fps_timer = 0.0
        __fps_frame_count = 0
        __fps = 0
    }

    static updateTitle(delta) {
        __fps_timer = __fps_timer + delta
        __fps_frame_count = __fps_frame_count + 1
        if (__fps_timer >= __fps_update_period) {
            __fps = (__fps_frame_count / __fps_timer).round
            __fps_timer = 0.0
            __fps_frame_count = 0
        }

        var pos = Renderer.cameraPosition
        var altitude = ((pos.x * pos.x + pos.y * pos.y + pos.z * pos.z).sqrt - __planet_radius).round

        Window.setTitle("Test Application - FPS: %(__fps) - altitude: %(altitude) m - speed: %(__camera_move_speed.round.round) m/s")
    }

    static update(delta) {
        updateTitle(delta)

        if (Input.keyPressed("[")) {
            __camera_move_speed = (__camera_move_speed / __camera_move_speed_factor).max(__camera_move_speed_min)
        }
        if (Input.keyPressed("]")) {
            __camera_move_speed = (__camera_move_speed * __camera_move_speed_factor).min(__camera_move_speed_max)
        }

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
