struct FrameUniforms {
    float4x4 inverse_projection;
    float4x4 camera_to_world;
    float2 resolution;
    float z_near;
    float fov_y;
    float aspect;
    float time;
    float2 _pad;
};

Texture2D<float4> scene_color;
Texture2D<float> scene_depth;
SamplerState scene_sampler;
ConstantBuffer<FrameUniforms> frame;

struct FullscreenOutput {
    float4 sv_position : SV_POSITION;
    float2 uv;
};

[shader("vertex")]
FullscreenOutput vertMain(uint vid: SV_VertexID) {
    float2 p = float2((vid << 1) & 2, vid & 2);
    FullscreenOutput o;
    o.sv_position = float4(p * 2.0 - 1.0, 0.0, 1.0);
    o.uv = float2(p.x, 1.0 - p.y);
    return o;
}

float linearizeDepth(float d) { return d > 0.0 ? frame.z_near / d : 1e30; }

float loadDepth(float2 uv) {
    int2 px = int2(uv * frame.resolution);
    return scene_depth.Load(int3(px, 0));
}
