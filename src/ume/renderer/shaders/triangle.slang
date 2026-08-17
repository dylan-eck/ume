struct DrawUniforms {
    float4x4 model_view_projection;
    float4x4 normal;
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

[shader("vertex")]
VertexOutput vertMain(uint vid: SV_VertexID) {
    VertexOutput output;
    output.sv_position =
        mul(draw.model_view_projection, vertices[vid].position);

    float4 normal = vertices[vid].normal;
    float3 normal_world = normalize(mul(draw.normal, normal).xyz);

    float3 normal_axis = float3(1, 0, 1);
    float nw_x = abs(normal.x);
    float nw_y = abs(normal.y);
    float nw_z = abs(normal.z);
    if (nw_x >= nw_y && nw_x >= nw_z) {
        normal_axis = float3(normal_axis.x, 0, 0);
    } else if (nw_y >= nw_x && nw_y >= nw_z) {
        normal_axis = float3(0, normal_axis.y, 0);
    } else if (nw_z > nw_x && nw_z > nw_y) {
        normal_axis = float3(0, 0, normal_axis.z);
    }

    normal_axis = normalize(normal_axis);

    output.normal = normal_world;
    // output.color = float4(normal_axis, 1);
    output.color = float4(vertices[vid].normal.xyz, 1);
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
    float3 dir_to_light = normalize(float3(1, 1, 1));

    float intensity = max(dot(input.normal, dir_to_light), 0.0) + 0.2;
    intensity = clamp(intensity, 0.0, 1.0);

    return float4(input.color.rgb * intensity, input.color.a);
}

