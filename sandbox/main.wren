import "ume" for Engine, Renderer

class Sandbox {
    static init() {
        __camera_distance = 80000000
        __fov_y = 15

        Renderer.setCamera(0, 0, __camera_distance, 0, 0, 0, __fov_y)

        __planet_1 = Engine.createObject(
            "proc_planet.Planet",
            {"radius": 7000000, "x": 0, "y": 0, "z": 0})

        // __planet_2 = Engine.createObject(
        //     "proc_planet.Planet",
        //     {"radius": 7000000, "x": 8000000, "y": 0, "z": 0})
    }

    static update(delta) {}
}
