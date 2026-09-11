struct DrawUniforms {
    float4x4 model_view_projection;
    float4x4 normal_matrix;
    float4 base_color;
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
    float3 normal_world =
        normalize(mul((float3x3)draw.normal_matrix, vertices[vid].normal.xyz));

    output.normal = normal_world;
    // output.color = draw.base_color;
    output.color = float4(1.0); 
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
    float3 n = normalize(input.normal);
    float3 dir_to_light = normalize(float3(0.5, 1, 0));

    const float ambient = 0.03;
    float intensity =
        ambient + (1.0 - ambient) * max(dot(n, dir_to_light), 0.0);

    return float4(input.color.rgb * intensity, input.color.a);
    // return float4(input.color.rgb, 1);
}

