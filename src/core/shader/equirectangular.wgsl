struct ColorParams {
    exposure: f32,
};

@group(0) @binding(0) var mySampler: sampler;
@group(0) @binding(1) var myTexture: texture_2d<f32>;
@group(0) @binding(2) var<uniform> params: ColorParams;

struct VertexOutput {
    @builtin(position) pos: vec4f,
    @location(0) fragDir: vec3f
};

@vertex
fn vs(@location(0) position: vec3f, @location(1) uv: vec2f) -> VertexOutput {
    var out: VertexOutput;
    out.pos = vec4f(position, 1.0);
    out.fragDir = normalize(position);
    return out;
}

@fragment
fn fs(input: VertexOutput) -> @location(0) vec4<f32> {
    let dir = input.fragDir;
    let u = atan2(dir.z, dir.x) / (2.0 * 3.1415926) + 0.5;
    let v = 0.5 - asin(dir.y) / 3.1415926;
    var color = textureSample(myTexture, mySampler, vec2f(u, v));
    color = color * params.exposure;
    return color;
}
