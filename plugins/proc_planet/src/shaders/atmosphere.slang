#include "ume/shaders/post-process.slang"

[shader("fragment")]
float4 fragMain(FullscreenOutput input) : SV_Target {
    float4 scene = scene_color.Sample(scene_sampler, input.uv);
    return float4(1.0 - scene.rgb, 1.0);
}
