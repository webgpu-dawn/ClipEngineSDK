// Custom Shader Example
// This is a simple grayscale shader that demonstrates external shader loading

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var myTexture : texture_2d<f32>;

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
    let color = textureSample(myTexture, mySampler, input.uv);

    // Convert to grayscale
    let gray = dot(color.rgb, vec3f(0.299, 0.587, 0.114));

    return vec4f(vec3f(gray), color.a);
}
