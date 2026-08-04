Main = {}
Main.t = 0
Main.camera_distance = 3

local function div(v, s)
    return { v[1] / s, v[2] / s, v[3] / s }
end

local function mag(v)
    return math.sqrt(v[1] * v[1] + v[2] * v[2] + v[3] * v[3])
end

local function norm(v)
    return div(v, mag(v))
end

local function cross(a, b)
    return {
        a[2] * b[3] - a[3] * b[2],
        a[3] * b[1] - a[1] * b[3],
        a[1] * b[2] - a[2] * b[1],
    }
end

local function cube_to_sphere(p)
    return {
        p[1] * math.sqrt(1 - p[2] * p[2] / 2 - p[3] * p[3] / 2 + p[2] * p[2] * p[3] * p[3] / 3),
        p[2] * math.sqrt(1 - p[3] * p[3] / 2 - p[1] * p[1] / 2 + p[3] * p[3] * p[1] * p[1] / 3),
        p[3] * math.sqrt(1 - p[1] * p[1] / 2 - p[2] * p[2] / 2 + p[1] * p[1] * p[2] * p[2] / 3)
    }
end

local function quad(normal, resolution)
    local num_quads = resolution * resolution

    local vertices = Ume.VertexArray.new(num_quads * 4)
    local indices = Ume.IndexArray.new(num_quads * 6)

    local n = normal
    local a = { n[2], n[3], n[1] }
    local b = cross(n, a)

    a[1] = 2 * a[1]
    a[2] = 2 * a[2]
    a[3] = 2 * a[3]

    b[1] = 2 * b[1]
    b[2] = 2 * b[2]
    b[3] = 2 * b[3]


    local vidx = 0
    local tidx = 0

    for i = 0, resolution - 1 do
        for j = 0, resolution - 1 do
            local t = i / resolution - 0.5
            local u = j / resolution - 0.5

            local p00 = cube_to_sphere({
                n[1] + t * a[1] + u * b[1],
                n[2] + t * a[2] + u * b[2],
                n[3] + t * a[3] + u * b[3],
            })

            local p10 = cube_to_sphere({
                n[1] + (t + 1 / resolution) * a[1] + u * b[1],
                n[2] + (t + 1 / resolution) * a[2] + u * b[2],
                n[3] + (t + 1 / resolution) * a[3] + u * b[3],
            })

            local p11 = cube_to_sphere({
                n[1] + (t + 1 / resolution) * a[1] + (u + 1 / resolution) * b[1],
                n[2] + (t + 1 / resolution) * a[2] + (u + 1 / resolution) * b[2],
                n[3] + (t + 1 / resolution) * a[3] + (u + 1 / resolution) * b[3],
            })

            local p01 = cube_to_sphere({
                n[1] + t * a[1] + (u + 1 / resolution) * b[1],
                n[2] + t * a[2] + (u + 1 / resolution) * b[2],
                n[3] + t * a[3] + (u + 1 / resolution) * b[3],
            })

            local n00 = norm(p00)
            local n10 = norm(p10)
            local n11 = norm(p11)
            local n01 = norm(p01)

            vertices:set_position(vidx, p00[1], p00[2], p00[3])
            vertices:set_normal(vidx, n00[1], n00[2], n00[3])

            vertices:set_position(vidx + 1, p10[1], p10[2], p10[3])
            vertices:set_normal(vidx + 1, n10[1], n10[2], n10[3])

            vertices:set_position(vidx + 2, p11[1], p11[2], p11[3])
            vertices:set_normal(vidx + 2, n11[1], n11[2], n11[3])

            vertices:set_position(vidx + 3, p01[1], p01[2], p01[3])
            vertices:set_normal(vidx + 3, n01[1], n01[2], n01[3])

            indices:set_triangle(tidx, vidx, vidx + 1, vidx + 2)
            indices:set_triangle(tidx + 1, vidx + 2, vidx + 3, vidx)

            vidx = vidx + 4
            tidx = tidx + 2
        end
    end

    return Ume.create_mesh(vertices, indices)
end

function Main.init()
    local res = 16

    Main.meshes = {
        quad({ 0, 0, 1 }, res),
        quad({ 0, 0, -1 }, res),
        quad({ 1, 0, 0 }, res),
        quad({ -1, 0, 0 }, res),
        quad({ 0, 1, 0 }, res),
        quad({ 0, -1, 0 }, res),
    }

    Ume.set_camera(0, 0, Main.camera_distance, 0, 0, 0, 45)
end

function Main.update(delta)
    Main.t = Main.t + 0.25 * delta
    local r = Main.camera_distance
    local x = r * math.cos(Main.t)
    local z = r * math.sin(Main.t)

    Ume.set_camera(x, 0, z, 0, 0, 0, 45)
    for i = 1, #Main.meshes do
        Ume.draw(Main.meshes[i])
    end
end
