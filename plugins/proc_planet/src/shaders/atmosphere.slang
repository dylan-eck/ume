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
    view_ray /= -view_ray.z;
    float3 view_pos = view_ray * linearizeDepth(loadDepth(uv));
    float3 world_pos = mul(frame.camera_to_world, float4(view_pos, 0.0)).xyz;

    float3 ray_dir =
        normalize(mul(frame.camera_to_world, float4(view_ray, 0.0)).xyz);
    float3 L = -params.center_and_radius.xyz; // camera at origin
    float a = dot(ray_dir, ray_dir);          // = 1 because unit vector
    float b = 2.0 * dot(ray_dir, L);
    float c =
        dot(L, L) - params.center_and_radius.w * params.center_and_radius.w;

    float discr = b * b - 4.0 * a * c;

    float t0;
    float t1;

    if (discr < 0) { // no intersection
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    if (discr == 0) {
        t0 = -0.5 * b / a;
        t1 = t0;
    } else {
        float q = (b > 0) ? -0.5 * (b + sqrt(discr)) : -0.5 * (b - sqrt(discr));
        t0 = q / a;
        t1 = c / q;
    }

    if (t0 > t1) {
        float tmp = t0;
        t0 = t1;
        t1 = tmp;
    }

    if (t0 < 0) {
        t0 = t1;

        if (t0 < 0) { // both points behind us
            return scene;
        }

        float intersect0 = length(t0 * ray_dir);
        float distThroughAtmos = intersect0;
        float distToPlanet = length(world_pos);

        if (distToPlanet < distThroughAtmos) {
            distThroughAtmos = distToPlanet;
        }

        return float4(float3(distThroughAtmos / 20000000), 1.0);
    }

    float3 intersect0 = t0 * ray_dir;
    float3 intersect1 = t1 * ray_dir;
    float distThroughAtmos = length(intersect1 - intersect0);

    float distToPlanet = length(world_pos);
    float distToAtmos =
        length(params.center_and_radius.xyz) - params.center_and_radius.w;

    if (distToPlanet < length(intersect1)) {
        distThroughAtmos = distToPlanet - distToAtmos;
    }

    return float4(float3(distThroughAtmos / 20000000), 1.0);
}
