struct DrawUniforms {
    float4x4 model_view_projection;
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
    float4 color;
};

[shader("vertex")]
VertexOutput vertMain(uint vid: SV_VertexID) {
    VertexOutput output;
    output.sv_position =
        mul(draw.model_view_projection, vertices[vid].position);
    output.color = float4(vertices[vid].normal.xyz * 0.5 + 0.5, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target { return input.color; }

