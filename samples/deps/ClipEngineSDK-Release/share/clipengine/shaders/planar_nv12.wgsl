// Planar NV12 Video Shader
// BT.709 color space conversion

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var yTex : texture_2d<f32>;
@group(0) @binding(2) var uvTex : texture_2d<f32>;

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
    let y = textureSample(yTex, mySampler, input.uv).r;
    let uv = textureSample(uvTex, mySampler, input.uv).rg;

    let u = uv.r - 0.5;
    let v = uv.g - 0.5;

    // BT.709 YUV to RGB conversion
    var rgb : vec3f;
    rgb.r = y + 1.5748 * v;
    rgb.g = y - 0.1873 * u - 0.4681 * v;
    rgb.b = y + 1.8556 * u;

    return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), 1.0);
}
