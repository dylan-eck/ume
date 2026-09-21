#include "ume/shaders/post-process.slang"

struct SunParams {
    float4 sun_dir;
}
ConstantBuffer<SunParams> params;

[shader("fragment")]
float4 fragMain(FullscreenOutput input) : SV_Target {
    float2 uv = input.uv;
    float4 scene = scene_texture.Sample(scene_sampler, uv);

    float3 ndc = float3(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0);
    float4 p = mul(frame.inverse_projection, float4(ndc, 1.0));
    float3 view_ray = p.xyz / p.w;
    view_ray /= -view_ray.z;
    float scene_dist = length(view_ray) * linearizeDepth(loadDepth(uv));

    float3 ray_dir =
        normalize(mul(frame.camera_to_world, float4(view_ray, 0.0)).xyz);

    float d = dot(ray_dir, params.sun_dir.xyz);

    // TODO: make these values make more sense?
    float t = smoothstep(0.99985, 0.9999, d);

    if (scene_dist == INF) {
        return lerp(scene, float4(1.0), t);
    }

    return scene;
}
