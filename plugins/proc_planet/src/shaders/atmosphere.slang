#include "ume/shaders/post-process.slang"

struct AtmosphereParams {
    float4 center_and_radius;
}
ConstantBuffer<AtmosphereParams> params;

[shader("fragment")]
float4 fragMain(FullscreenOutput input) : SV_Target {
    float2 uv = input.uv;
    float4 scene = scene_color.Sample(scene_sampler, uv);

    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float4 p = mul(frame.inverse_projection, float4(ndc, 1.0, 1.0));
    float3 view_ray = p.xyz / p.w;
    // division by -view_ray.z makes view_ray.z = -1
    // the camera looks along the -z axis, so view_ray is dx and dy per unit
    // depth after the division
    view_ray /= -view_ray.z;
    float scene_dist = length(view_ray) * linearizeDepth(loadDepth(uv));

    float3 ray_dir =
        normalize(mul(frame.camera_to_world, float4(view_ray, 0.0)).xyz);

    float3 oc = -params.center_and_radius.xyz;
    float radius = params.center_and_radius.w;
    float b = dot(ray_dir, oc);
    float discr = b * b - (dot(oc, oc) - radius * radius);
    if (discr < 0.0)
        return scene;

    float s = sqrt(discr);
    float near = max(-b - s, 0.0);
    float far = min(-b + s, scene_dist);
    if (far <= near)
        return scene;

    const float atmosphere_scale = 1.0 / 20000000.0;
    return float4(float3((far - near) * atmosphere_scale), 1.0);
}
