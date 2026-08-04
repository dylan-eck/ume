Main = {}
Main.t = 0

local function quad(normal, resolution) -- px, py, pz, nx, ny, nz,
    local num_quads = resolution * resolution

    local v = Ume.VertexArray.new(num_quads * 4)
    local i = Ume.IndexArray.new(num_quads * 6)

    local n = normal
    local a = { n[2], n[3], n[1] }
    local b = {
        a[2] * n[3] - a[3] * n[2],
        a[1] * n[3] - a[3] * n[1],
        a[1] * n[2] - a[2] * n[1],
    }

    local vidx = 0
    local tidx = 0

    for t = 0, resolution - 1 do
        for u = 0, resolution - 1 do
            local p00 = {
                -1 + n[1] + t * a[1] + u * b[1],
                -1 + n[2] + t * a[2] + u * b[2],
                -1 + n[3] + t * a[3] + u * b[3],
            }

            local p10 = {
                -1 + n[1] + (t + 1) * a[1] + u * b[1],
                -1 + n[2] + (t + 1) * a[2] + u * b[2],
                -1 + n[3] + (t + 1) * a[3] + u * b[3],
            }

            local p11 = {
                -1 + n[1] + (t + 1) * a[1] + (u + 1) * b[1],
                -1 + n[2] + (t + 1) * a[2] + (u + 1) * b[2],
                -1 + n[3] + (t + 1) * a[3] + (u + 1) * b[3],
            }

            local p01 = {
                -1 + n[1] + t * a[1] + (u + 1) * b[1],
                -1 + n[2] + t * a[2] + (u + 1) * b[2],
                -1 + n[3] + t * a[3] + (u + 1) * b[3],
            }



            v:set_position(vidx, p00[1], p00[2], p00[3])
            v:set_normal(vidx, 0, 0, 1)

            v:set_position(vidx + 1, p10[1], p10[2], p10[3])
            v:set_normal(vidx + 1, 0, 0, 1)

            v:set_position(vidx + 2, p11[1], p11[2], p11[3])
            v:set_normal(vidx + 2, 0, 0, 1)

            v:set_position(vidx + 3, p01[1], p01[2], p01[3])
            v:set_normal(vidx + 3, 0, 0, 1)

            i:set_triangle(tidx, vidx, vidx + 1, vidx + 2)
            i:set_triangle(tidx + 1, vidx + 2, vidx + 3, vidx)

            vidx = vidx + 4
            tidx = tidx + 2
        end
    end

    return Ume.create_mesh(v, i)
end

function Main.init()
    Main.mesh = quad({ 0, 0, 1 }, 8)

    Ume.set_camera(0, 0, 6, 0, 0, 0, 45)
end

function Main.update(delta)
    -- Main.t = Main.t + 0.25 * delta
    -- local r = 6
    -- local x = r * math.cos(Main.t)
    -- local z = r * math.sin(Main.t)

    -- Ume.set_camera(x, 6, z, 0, 0, 0, 45)
    Ume.draw(Main.mesh)
end
