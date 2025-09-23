struct Uniforms {
    viewProj: mat4x4<f32>
}
@group(0) @binding(0) var<uniform> uUniforms : Uniforms;

@group(0) @binding(1) var uSampler : sampler;
@group(0) @binding(2) var uTexture : texture_2d<f32>;

struct VSOut {
    @builtin(position) pos: vec4<f32>,
    @location(0) dir: vec3<f32>
};

@vertex
fn vs(@builtin(vertex_index) vid: u32) -> VSOut {
    var pos = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>(3.0, -1.0),
        vec2<f32>(-1.0, 3.0)
    );

    var out: VSOut;
    out.pos = vec4<f32>(pos[vid], 0.0, 1.0);

    // 屏幕坐标转为球面方向
    let ndc = pos[vid];
    let dir = normalize(vec3<f32>(ndc.x, ndc.y, -1.0));
    out.dir = (uUniforms.viewProj * vec4<f32>(dir, 0.0)).xyz;
    return out;
}

@fragment
fn fs(in: VSOut) -> @location(0) vec4<f32> {
    let d = normalize(in.dir);
    let lon = atan2(d.z, d.x);
    let lat = asin(d.y);

    var uv = vec2<f32>(
        (lon / (2.0 * 3.1415926535)) + 0.5,
        (lat / 3.1415926535) + 0.5
    );

    return textureSample(uTexture, uSampler, uv);
}