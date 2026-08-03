Main = {}
Main.t = 0

local function quad(resolution)
    local v = Ume.VertexArray.new(500)
    local i = Ume.IndexArray.new(500)

    local vidx = 0
    local tidx = 0

    local start = -1
    local step = 2 / resolution

    for k = 0, resolution - 1 do
        for j = 0, resolution - 1 do
            local x0 = start + j * step
            local y0 = start + k * step

            local x1 = start + (j + 1) * step
            local y1 = start + (k + 1) * step

            v:set_position(vidx, x0, y0, 0)
            v:set_normal(vidx, 0, 0, 1)

            v:set_position(vidx + 1, x1, y0, 0)
            v:set_normal(vidx + 1, 0, 0, 1)

            v:set_position(vidx + 2, x1, y1, 0)
            v:set_normal(vidx + 2, 0, 0, 1)

            v:set_position(vidx + 3, x0, y1, 0)
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
    Main.mesh = quad(4)

    Ume.set_camera(0, 0, 3, 0, 0, 0, 45)
end

function Main.update(delta)
    -- Main.t = Main.t + 0.5 * delta
    -- local r = 3
    -- local x = r * math.cos(Main.t)
    -- local z = r * math.sin(Main.t)

    -- Ume.set_camera(x, 3, z, 0, 0, 0, 45)
    Ume.draw(Main.mesh)
end
