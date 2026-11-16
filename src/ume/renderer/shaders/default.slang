#include "ume/shaders/planet_lighting.slang"

struct DrawUniforms {
    float4x4 model_view_projection;
    float4x4 model; // mesh local -> camera-relative world
    float4x4 normal_matrix;
    float4 base_color;
    float4 world_origin; // world origin in camera-relative space
};

[[vk::push_constant]]
ConstantBuffer<DrawUniforms> draw;

struct Vertex {
    float4 position;
    float4 normal;
};

StructuredBuffer<Vertex> vertices;

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 world_position;
    float3 normal;
    float4 color;
};

// Temporary: atmosphere parameters duplicated from the proc_planet plugin so
// surface lighting can account for the atmosphere. These should eventually
// come from the planet.
static const float planet_radius = 250;
static const float atmosphere_thickness = 100;
static const PlanetAtmosphere atmosphere = {
    float3(0, 0, 0), // planet center, world space
    planet_radius,
    atmosphere_thickness,
    1.0 / atmosphere_thickness, // scattering strength, 1 / m
};

static const float4 beachColor = float4(1, 0.996, 0.753, 1);
static const float4 grassColor = float4(0.608, 0.851, 0.29, 1);
static const float4 mountainColor = float4(0.063, 0.478, 0.071, 1);
static const float4 peakColor = float4(0.471, 0.49, 0.443, 1);
static const float4 highPeakColor = float4(0.8, 0.8, 0.8, 1);

struct ColorStop {
    float4 color;
    float stop;
};

static const int numColors = 6;
static const ColorStop colors[numColors] = {
    { beachColor, 0 },      { beachColor, 0.45 }, { grassColor, 0.51 },
    { mountainColor, 0.6 }, { peakColor, 0.8 },   { highPeakColor, 0.95 },
};

// static const float4 terrain_colors[numColors] = {
//     beachColor, grassColor, mountainColor, peakColor, highPeakColor,
// };

[shader("vertex")]
VertexOutput vertMain(uint vid: SV_VertexID) {
    VertexOutput output;
    output.sv_position =
        mul(draw.model_view_projection, vertices[vid].position);

    float3 camera_relative = mul(draw.model, vertices[vid].position).xyz;
    output.world_position = camera_relative - draw.world_origin.xyz;

    float4 normal = vertices[vid].normal;
    float3 normal_world =
        normalize(mul((float3x3)draw.normal_matrix, vertices[vid].normal.xyz));

    output.normal = normal_world;
    // output.color = draw.base_color;
    output.color = float4(0.2, 0.8, 0.4, 1.0);
    output.color = float4(0.17832, 0.41773, 0.10553, 1.0);
    // output.color = float4(1.0);
    // output.color = float4(0.01, 0.1, 0.1, 1.0);
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
    float3 n = normalize(input.normal);
    float3 intensity =
        planet_surface_lighting(atmosphere, input.world_position, n);

    float alt = ((length(input.world_position.xyz) - 250) + 20) / 50;
    alt = clamp(alt, 0, 1);

    float4 color;

    // if (alt == 0) {
    //     color = terrain_colors[0];
    // } else if (alt == 1) {
    //     color = terrain_colors[numColors - 1];
    // } else {
    //     int idx = int(alt * (numColors - 1));

    //     float4 colorA = terrain_colors[idx];
    //     float4 colorB = terrain_colors[idx + 1];

    //     float t = (alt - 1.0 / (numColors - 1) * idx) * (numColors - 1);
    //     color = lerp(colorA, colorB, t);
    // }

    ColorStop stopA = colors[0];
    int next_index = 1;
    for (int i = 1; i < numColors; i++) {
        if (alt >= colors[i].stop) {
            stopA = colors[i];
            next_index++;
        } else {
            break;
        }
    }

    if (next_index == numColors) {
        color = stopA.color;
    } else {
        ColorStop stopB = colors[next_index];

        float range = stopB.stop - stopA.stop;
        float t = (alt - stopA.stop) / range;

        color = lerp(stopA.color, stopB.color, t);
    }

    return float4(color.rgb * intensity, 1.0);
}

