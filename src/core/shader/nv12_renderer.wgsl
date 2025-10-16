// NV12 YUV 转 RGB 渲染器
// NV12 格式: Plane0 = Y (R8), Plane1 = UV (RG8)

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var y_tex : texture_2d<f32>;   // Y plane (R8Unorm)
@group(0) @binding(2) var uv_tex : texture_2d<f32>;  // UV plane (RG8Unorm)

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
    // 采样 Y 和 UV 分量
    let y = textureSample(y_tex, mySampler, input.fragUV).r;
    let uv = textureSample(uv_tex, mySampler, input.fragUV).rg;

    // UV 分量解包
    let u = uv.r;
    let v = uv.g;

    // YUV 转 RGB (BT.709 标准)
    // Y 范围 [0, 1], UV 范围 [0, 1] 需要转换到 [-0.5, 0.5]
    let y_adjusted = y;
    let u_adjusted = u - 0.5;
    let v_adjusted = v - 0.5;

    // BT.709 转换矩阵
    var rgb : vec3f;
    rgb.r = y_adjusted + 1.5748 * v_adjusted;
    rgb.g = y_adjusted - 0.1873 * u_adjusted - 0.4681 * v_adjusted;
    rgb.b = y_adjusted + 1.8556 * u_adjusted;

    // 钳制到 [0, 1]
    rgb = clamp(rgb, vec3f(0.0), vec3f(1.0));

    return vec4f(rgb, 1.0);
}
