#include "ume/shaders/post-process.slang"

struct AtmosphereParams {
    float4 center_and_radius;
}
ConstantBuffer<AtmosphereParams> params;

static const float3 sun_dir = normalize(float3(1.0, 0.0, 0.0));

static const int in_scattering_samples = 10;
static const int optical_depth_samples = 10;
static const float density_falloff = 2.0;
static const float scattering_strength = 1.17e-7; // 1 / m

float2 ray_sphere_intersect(float3 center, float radius, float3 ray_origin,
                            float3 ray_dir) {

    float3 oc = ray_origin - center;
    float b = dot(ray_dir, oc);
    float discr = b * b - (dot(oc, oc) - radius * radius);

    if (discr <= 0) {
        return float2(-1.0, -1.0);
    }

    float s = sqrt(discr);
    float near = max(-b - s, 0.0);
    float far = -b + s;

    if (far <= near) {
        return float2(-1.0, -1.0);
    }

    return float2(near, far - near);
}

float density_at_point(float3 point) {
    float height_above_surface = length(point - params.center_and_radius.xyz) -
                                 7e6; // TODO: pass planet radius to shader
    float height01 = height_above_surface / (params.center_and_radius.w - 7e6);
    float local_density = exp(-height01 * density_falloff) * (1.0 - height01);
    return local_density;
}

float optical_depth(float3 ray_origin, float3 ray_dir, float ray_length) {
    float3 curr_point = ray_origin;
    float step_size = ray_length / (optical_depth_samples - 1);
    float optical_depth = 0;

    for (int i = 0; i < optical_depth_samples; i++) {
        float density = density_at_point(curr_point);
        optical_depth += density * step_size;
        curr_point += ray_dir * step_size;
    }
    return optical_depth;
}

float calculateLight(float3 ray_origin, float3 ray_dir, float ray_length) {
    float3 curr_point = ray_origin;
    float step_size = ray_length / (in_scattering_samples - 1);
    float in_scattered_light = 0;

    float3 planet_center = params.center_and_radius.xyz;
    float atmosphere_radius = params.center_and_radius.w;

    for (int i = 0; i < in_scattering_samples; i++) {
        float sun_ray_length =
            ray_sphere_intersect(planet_center, atmosphere_radius, curr_point,
                                 sun_dir)
                .y;

        float sun_ray_optical_depth =
            scattering_strength *
            optical_depth(curr_point, sun_dir, sun_ray_length);

        float view_ray_optical_depth =
            scattering_strength *
            optical_depth(curr_point, -ray_dir, step_size * i);

        float transmittance =
            exp(-(sun_ray_optical_depth + view_ray_optical_depth));
        float local_density = density_at_point(curr_point);

        in_scattered_light += local_density * transmittance * step_size;
        curr_point += ray_dir * step_size;
    }

    return scattering_strength * in_scattered_light;
}

[shader("fragment")]
float4 fragMain(FullscreenOutput input) : SV_Target {
    float2 uv = input.uv;
    float4 scene = scene_color.Sample(scene_sampler, uv);

    float3 ndc = float3(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0);
    float4 p = mul(frame.inverse_projection, float4(ndc, 1.0));
    float3 view_ray = p.xyz / p.w;
    view_ray /= -view_ray.z;
    float scene_dist = length(view_ray) * linearizeDepth(loadDepth(uv));

    float3 ray_dir =
        normalize(mul(frame.camera_to_world, float4(view_ray, 0.0)).xyz);

    float3 planet_center = params.center_and_radius.xyz;
    float atmosphere_radius = params.center_and_radius.w;

    float2 intersect = ray_sphere_intersect(planet_center, atmosphere_radius,
                                            float3(0), ray_dir);

    float dist_to_atmos = intersect.x;
    float dist_through_atmos = min(intersect.y, scene_dist - dist_to_atmos);

    if (dist_through_atmos <= 0) {
        return scene;
    }

    float3 point_in_atmosphere = ray_dir * dist_to_atmos;
    float light =
        calculateLight(point_in_atmosphere, ray_dir, dist_through_atmos);

    // const float atmosphere_scale = 1.0 / 20000000.0;
    // return float4(float3(dist_through_atmos * atmosphere_scale), 1.0);
    return (scene * (1 - light) + light);
}
