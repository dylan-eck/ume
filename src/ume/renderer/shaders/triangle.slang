struct Vertex {
    float4 position;
    float4 color;
};

StructuredBuffer<Vertex> vertices;

struct VertexOutput {
    float4 sv_position : SV_Position;
    float4 color;
};

[shader("vertex")]
VertexOutput vertMain(uint vid: SV_VertexID) {
    VertexOutput output;
    output.sv_position = vertices[vid].position;
    output.color = vertices[vid].color;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target { return input.color; }

