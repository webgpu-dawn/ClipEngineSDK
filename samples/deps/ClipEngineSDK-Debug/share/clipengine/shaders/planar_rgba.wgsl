// Planar RGBA Texture Shader

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var myTexture : texture_2d<f32>;

struct VertexOutput {
    @builtin(position) pos : vec4f,
    @location(0) uv : vec2f
};

@vertex
fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput {
    var out : VertexOutput;
    out.pos = vec4f(pos, 0.0, 1.0);
    out.uv = uv;
    return out;
}

@fragment
fn fs(input : VertexOutput) -> @location(0) vec4f {
    return textureSample(myTexture, mySampler, input.uv);
}
