// 只渲染 Y 分量的简化 shader

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var y_tex : texture_2d<f32>;

struct VertexOutput
{
    @builtin(position) pos : vec4f,
    @location(0) fragUV : vec2f
};

@vertex
fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f) -> VertexOutput
{
    var out : VertexOutput;
    out.pos = vec4f(pos, 0.0, 1.0);
    out.fragUV = uv;
    return out;
}

@fragment
fn fs(input : VertexOutput) -> @location(0) vec4f
{
    // 采样 Y 分量
    let y = textureSample(y_tex, mySampler, input.fragUV).r;

    // 将 Y 分量作为灰度值输出
    return vec4f(y, y, y, 1.0);
}
