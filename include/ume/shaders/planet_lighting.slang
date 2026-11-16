// Surface lighting for a planet wrapped in a Rayleigh-scattering atmosphere.
//
// Direct sunlight is attenuated by the optical depth of the sun ray through the
// atmosphere, so it reddens toward the terminator. The light scattered out of
// the beam is partly returned to the surface as blue-tinted skylight, which
// keeps the direct and sky terms roughly energy conserving.
//
// All positions are in whatever space the atmosphere's center is given in, so
// the same functions work in world space (terrain) and camera-relative space
// (post effects).

static const float3 planet_sun_dir = normalize(float3(1, 0, 0));

static const float planet_density_falloff = 2.0;
static const int planet_optical_depth_samples = 10;
static const float3 planet_wavelengths = float3(700, 530, 400);

// Fraction of the sunlight scattered out of the direct beam that ends up
// coming back down as skylight. Rayleigh scattering is symmetric forward and
// back, so about half of what leaves the beam is redirected toward the ground.
static const float planet_sky_return_fraction = 0.5;

struct PlanetAtmosphere {
    float3 center;
    float planet_radius;
    float thickness;
    // Scattering strength in 1 / m at the most strongly scattered (blue)
    // wavelength.
    float scattering_strength;

    float atmosphere_radius() { return planet_radius + thickness; }

    float3 scattering_coefficients() {
        return scattering_strength * float3(pow(400 / planet_wavelengths, 4));
    }
};

// Distance along ray_dir from ray_origin (assumed inside the sphere) to the
// sphere surface.
float planet_ray_sphere_exit(float3 center, float radius, float3 ray_origin,
                             float3 ray_dir) {
    float3 oc = ray_origin - center;
    float b = dot(ray_dir, oc);
    float discr = b * b - (dot(oc, oc) - radius * radius);
    if (discr <= 0) return 0.0;
    return max(-b + sqrt(discr), 0.0);
}

float planet_density_at_point(PlanetAtmosphere atmos, float3 point) {
    float height_above_surface =
        length(point - atmos.center) - atmos.planet_radius;
    float height01 = saturate(height_above_surface / atmos.thickness);
    return exp(-height01 * planet_density_falloff) * (1.0 - height01);
}

float planet_optical_depth(PlanetAtmosphere atmos, float3 ray_origin,
                           float3 ray_dir, float ray_length) {
    float3 curr_point = ray_origin;
    float step_size = ray_length / (planet_optical_depth_samples - 1);
    float depth = 0;
    for (int i = 0; i < planet_optical_depth_samples; i++) {
        depth += planet_density_at_point(atmos, curr_point) * step_size;
        curr_point += ray_dir * step_size;
    }
    return depth;
}

// Fraction of sunlight, per wavelength, that reaches a point on the surface
// after out-scattering along the sun ray through the atmosphere.
float3 planet_sun_transmittance(PlanetAtmosphere atmos, float3 position) {
    float ray_length = planet_ray_sphere_exit(
        atmos.center, atmos.atmosphere_radius(), position, planet_sun_dir);
    float depth =
        planet_optical_depth(atmos, position, planet_sun_dir, ray_length);
    return exp(-depth * atmos.scattering_coefficients());
}

// Sunlight transmittance straight up through the atmosphere. This is the same
// for every point on the surface, so it only depends on the profile.
float3 planet_zenith_transmittance(PlanetAtmosphere atmos) {
    float3 up = float3(0, 1, 0);
    float3 surface_point = atmos.center + up * atmos.planet_radius;
    float depth =
        planet_optical_depth(atmos, surface_point, up, atmos.thickness);
    return exp(-depth * atmos.scattering_coefficients());
}

// Light in-scattered by the sky toward the ground. Whatever the atmosphere
// removes from the direct beam on the way down it has to scatter somewhere,
// and a fraction of that comes back to the surface as skylight. Using one
// minus the zenith transmittance as the sky radiance keeps the direct and sky
// terms roughly energy conserving: the blue that planet_sun_transmittance
// strips out is what shows up here.
//
// The sky is a hemisphere, so a surface sees more of it the closer its normal
// is to straight up. It fades out with the sun's elevation at the surface
// point so the night side is not lit by a sky that is not receiving any
// sunlight.
float3 planet_sky_ambient(PlanetAtmosphere atmos, float3 position,
                          float3 normal) {
    float3 sky_radiance = (1.0 - planet_zenith_transmittance(atmos)) *
                          planet_sky_return_fraction;
    float3 up = normalize(position - atmos.center);
    float sky_visibility = 0.5 * (1.0 + dot(normal, up));
    // Sky stays lit a little past the geometric terminator since the
    // atmosphere above the point is still sunlit.
    float daylight = smoothstep(-0.2, 0.2, dot(up, planet_sun_dir));
    return sky_radiance * sky_visibility * daylight;
}

// Total irradiance, per wavelength, on a surface at position with the given
// normal: attenuated direct sunlight plus skylight.
float3 planet_surface_lighting(PlanetAtmosphere atmos, float3 position,
                               float3 normal) {
    float3 direct = planet_sun_transmittance(atmos, position) *
                    max(dot(normal, planet_sun_dir), 0.0);
    return planet_sky_ambient(atmos, position, normal) + direct;
}
