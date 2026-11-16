#include "ume/shaders/post-process.slang"
#include "ume/shaders/planet_lighting.slang"

struct OceanParams {
    // xyz: planet center relative to the camera, w: ocean radius
    float4 center_and_radius;
    // x: planet radius, y: atmosphere thickness (atmosphere radius - planet
    // radius), z: scattering strength in 1 / m, w: unused
    float4 planet_radius_thickness_strength;
}
ConstantBuffer<OceanParams> params;

static const float4 shallow_color = float4(0.48584, 0.92041, 0.98823, 1.0);
// float4(0.15076, 0.36171, 0.96333, 1.0);
//  float4(0.39499, 0.8567, 0.75526, 1.0); //
static const float4 deep_color = float4(0.012666, 0.063608, 0.75833, 1.0);
static const float depth_multiplier = 0.1;
static const float alpha_multiplier = 100;

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

    float3 planet_center = params.center_and_radius.xyz;
    float ocean_radius = params.center_and_radius.w;

    float2 intersect =
        ray_sphere_intersect(planet_center, ocean_radius, float3(0), ray_dir);

    float dist_to_ocean = intersect.x;
    float dist_through_ocean = min(intersect.y, scene_dist - dist_to_ocean);

    if (dist_through_ocean > 0) {
        float optical_depth = 1 - exp(-dist_through_ocean * depth_multiplier);
        float alpha = 1 - exp(-dist_through_ocean * alpha_multiplier);
        float4 ocean_color = lerp(shallow_color, deep_color, optical_depth);

        // Light the water surface with the same model as the terrain so the
        // two match across the shoreline. Everything here is
        // camera-relative, which is fine since the model only uses
        // positions relative to the planet center.
        PlanetAtmosphere atmosphere = {
            planet_center,
            params.planet_radius_thickness_strength.x,
            params.planet_radius_thickness_strength.y,
            params.planet_radius_thickness_strength.z,
        };
        float3 surface_point = ray_dir * dist_to_ocean;
        float3 normal = normalize(surface_point - planet_center);
        ocean_color.rgb *=
            planet_surface_lighting(atmosphere, surface_point, normal);

        return lerp(scene, ocean_color, alpha);
    }

    return scene;
}
