StructuredBuffer<float> input;
RWStructuredBuffer<float> output;

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 world_position;
    float3 normal;
    float4 color;
};

[shader("compute")]
[numthreads(64, 1, 1)]
void doubleArray(uint3 tid: SV_DispatchThreadID) {
    output[tid.x] = input[tid.x] * 2.0;
}

[shader("compute")]
[numthreads(35, 35, 1)]
void generateHeight(uint3 tid: SV_DispatchThreadID) {}

[shader("compute")]
[numthreads(1, 1, 1)]
void generateNormalMap(uint3 tid: SV_DispatchThreadID) {}

// [shader("vertex")]
// VertexOutput vertMain(uint vid: SV_VertexID) {
//     VertexOutput output;
//     return output;
// }

// [shader("fragment")]
// float4 fragMain(VertexOutput input) : SV_Target {
//     return float4(input.color.rgb, 1);
// }

