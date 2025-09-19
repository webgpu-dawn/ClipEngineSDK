
struct ColorParams
{
    exposure : f32,
    contrast : f32
};

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var rgba_tex : texture_2d<f32>;
@group(0) @binding(2) var<uniform> params : ColorParams;

struct VertexOutput
{
    @builtin(position) pos : vec4f,
    @location(0) fragUV : vec2f
};

@vertex
    fn vs(@location(0) pos : vec2f, @location(1) uv : vec2f)
        ->VertexOutput
{
    var out : VertexOutput;
    out.pos = vec4f(pos, 0.0, 1.0);
    out.fragUV = uv;
    return out;
}

@fragment
    fn fs(input : VertexOutput) -> @location(0) vec4f
{
    var color = textureSample(rgba_tex, mySampler, input.fragUV);

    var rgb = color.rgb;
    rgb *= params.exposure;
    rgb = (rgb - 0.5) * params.contrast + 0.5;

    // 限制范围
    rgb = clamp(rgb, vec3f(0.0), vec3f(1.0));

    return vec4f(rgb, color.a);
}