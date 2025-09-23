// binding group 0
@group(0) @binding(0) var mySampler: sampler;
@group(0) @binding(1) var myTexture: texture_2d<f32>;

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

// 片元着色器：直接采样 RGBA 纹理
@fragment
fn fs(input: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(myTexture, mySampler, input.fragUV);
}
