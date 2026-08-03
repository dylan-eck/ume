Main = {}
Main.t = 0

local CUBE_FACES = {
    { n = { 0, 0, 1 },  p = { { -0.5, -0.5, 0.5 }, { 0.5, -0.5, 0.5 }, { 0.5, 0.5, 0.5 }, { -0.5, 0.5, 0.5 } } },
    { n = { 0, 0, -1 }, p = { { 0.5, -0.5, -0.5 }, { -0.5, -0.5, -0.5 }, { -0.5, 0.5, -0.5 }, { 0.5, 0.5, -0.5 } } },
    { n = { -1, 0, 0 }, p = { { -0.5, -0.5, -0.5 }, { -0.5, -0.5, 0.5 }, { -0.5, 0.5, 0.5 }, { -0.5, 0.5, -0.5 } } },
    { n = { 1, 0, 0 },  p = { { 0.5, -0.5, 0.5 }, { 0.5, -0.5, -0.5 }, { 0.5, 0.5, -0.5 }, { 0.5, 0.5, 0.5 } } },
    { n = { 0, 1, 0 },  p = { { -0.5, 0.5, 0.5 }, { 0.5, 0.5, 0.5 }, { 0.5, 0.5, -0.5 }, { -0.5, 0.5, -0.5 } } },
    { n = { 0, -1, 0 }, p = { { -0.5, -0.5, -0.5 }, { 0.5, -0.5, -0.5 }, { 0.5, -0.5, 0.5 }, { -0.5, -0.5, 0.5 } } },
}

local function make_cube()
    local v = Ume.VertexArray.new(#CUBE_FACES * 4)
    local idx = Ume.IndexArray.new(#CUBE_FACES * 6)

    for f = 1, #CUBE_FACES do
        local face = CUBE_FACES[f]
        local base = (f - 1) * 4
        local tri  = (f - 1) * 2

        for c = 1, 4 do
            local p = face.p[c]
            v:set_position(base + c - 1, p[1], p[2], p[3])
            v:set_normal(base + c - 1, face.n[1], face.n[2], face.n[3])
        end

        idx:set_triangle(tri, base, base + 1, base + 2)
        idx:set_triangle(tri + 1, base, base + 2, base + 3)
    end

    return Ume.create_mesh(v, idx)
end

function Main.init()
    Main.mesh = make_cube()
    Ume.set_camera(0, 2, 3, 0, 0, 0, 45)
end

function Main.update(delta)
    Main.t = Main.t + delta
    local r = 3
    local x = r * math.cos(Main.t)
    local z = r * math.sin(Main.t)

    Ume.set_camera(x, 2, z, 0, 0, 0, 45)
    Ume.draw(Main.mesh)
end
