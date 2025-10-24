// Color Adjustment Effect
// Demonstrates using uniform parameters with external shader

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var myTexture : texture_2d<f32>;
@group(0) @binding(2) var<uniform> params : vec4f; // brightness, contrast, saturation, hue

struct VertexInput {
    @location(0) position : vec2f,
    @location(1) uv : vec2f
}

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) uv : vec2f
}

@vertex
fn vs_main(input : VertexInput) -> VertexOutput {
    var output : VertexOutput;
    output.position = vec4f(input.position, 0.0, 1.0);
    output.uv = input.uv;
    return output;
}

@fragment
fn fs_main(input : VertexOutput) -> @location(0) vec4f {
    // Sample the texture
    var color = textureSample(myTexture, mySampler, input.uv);

    // Apply brightness (params.x)
    color.rgb += params.x;

    // Apply contrast (params.y)
    color.rgb = (color.rgb - 0.5) * params.y + 0.5;

    // Apply saturation (params.z)
    let gray = dot(color.rgb, vec3f(0.299, 0.587, 0.114));
    color.rgb = mix(vec3f(gray), color.rgb, params.z);

    return color;
}
