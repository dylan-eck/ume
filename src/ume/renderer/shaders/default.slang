struct DrawUniforms {
    float4x4 model_view_projection;
    float4x4 normal_matrix;
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
    float3 normal_world = normalize(mul(draw.normal_matrix, normal).xyz);

    output.normal = normal_world;
    output.color = normal;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
    float3 dir_to_light = normalize(float3(1, 0, 0));

    float intensity = max(dot(input.normal, dir_to_light), 0.0) + 0.05;
    intensity = clamp(intensity, 0.0, 1.0);

    // return float4(input.color.rgb * intensity, input.color.a);
    return float4(input.color.rgb, 1);
}

