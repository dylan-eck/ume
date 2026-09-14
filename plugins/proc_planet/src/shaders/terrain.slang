struct TerrainParams {
    float2 origin;
    float size;
    uint grid_width;
};

[[vk::push_constant]]
ConstantBuffer<TerrainParams> params;

RWStructuredBuffer<float> heights;

[shader("compute")]
[numthreads(8, 8, 1)]
void generateTerrain(uint3 tid: SV_DispatchThreadID) {
    float h = tid.x;
    heights[tid.y * params.grid_width + tid.x] = h;
}
