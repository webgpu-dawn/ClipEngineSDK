struct ColorParams {
    exposure: f32,
};

@group(0) @binding(0) var mySampler: sampler;
@group(0) @binding(1) var myTexture: texture_2d<f32>;
@group(0) @binding(2) var<uniform> params: ColorParams;

struct VertexOutput {
    @builtin(position) pos: vec4f,
    @location(0) fragUV: vec2f
};

@vertex
fn vs(@location(0) pos: vec2f, @location(1) uv: vec2f) -> VertexOutput {
    var out: VertexOutput;
    out.pos = vec4f(pos, 0.0, 1.0);
    out.fragUV = uv;
    return out;
}

@fragment
fn fs(input: VertexOutput) -> @location(0) vec4<f32> {
    // 计算球面坐标
    let ndc = input.fragUV * 2.0 - vec2f(1.0, 1.0);
    let theta = ndc.y * 0.5 * 3.1415926;
    let phi = ndc.x * 2.0 * 3.1415926;

    let dir = vec3f(
        cos(theta) * sin(phi),
        sin(theta),
        cos(theta) * cos(phi)
    );

    let u = atan2(dir.z, dir.x) / (2.0 * 3.1415926) + 0.5;
    let v = dir.y * 0.5 + 0.5;

    var color = textureSample(myTexture, mySampler, vec2f(u, v));
    color = color * params.exposure; // uniform buffer
    return color;
}
