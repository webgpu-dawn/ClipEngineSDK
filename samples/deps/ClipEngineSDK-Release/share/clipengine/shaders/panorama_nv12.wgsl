// Panorama NV12 Shader (360° equirectangular)
// BT.709 color space conversion with spherical mapping

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var yTex : texture_2d<f32>;
@group(0) @binding(2) var uvTex : texture_2d<f32>;
@group(0) @binding(3) var<uniform> u : vec4f; // yaw, pitch, zoom, aspect

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

fn toSpherical(uv: vec2f, yaw: f32, pitch: f32, zoom: f32, aspect: f32) -> vec2f {
    var x = (uv.x - 0.5) * aspect;
    var y = (uv.y - 0.5);
    x = x / zoom;
    y = y / zoom;
    var dir = vec3f(x, y, -1.0);
    dir = normalize(dir);
    let cy = cos(yaw);
    let sy = sin(yaw);
    let cx = cos(pitch);
    let sx = sin(pitch);
    var rx = vec3f(dir.x, dir.y * cx - dir.z * sx, dir.y * sx + dir.z * cx);
    var r = vec3f(rx.x * cy + rx.z * sy, rx.y, -rx.x * sy + rx.z * cy);
    let lon = atan2(r.x, -r.z);
    let lat = asin(clamp(r.y, -1.0, 1.0));
    var uout = lon / (2.0 * 3.14159265) + 0.5;
    var vout = 0.5 - lat / 3.14159265;
    return vec2f(fract(uout), clamp(1.0 - vout, 0.0, 1.0));
}

@fragment
fn fs(input : VertexOutput) -> @location(0) vec4f {
    let yaw = u.x;
    let pitch = u.y;
    let zoom = max(u.z, 0.01);
    let aspect = max(u.w, 1.0);
    let sphUV = toSpherical(input.uv, yaw, pitch, zoom, aspect);

    let y = textureSample(yTex, mySampler, sphUV).r;
    let uv = textureSample(uvTex, mySampler, sphUV).rg;
    let uval = uv.r - 0.5;
    let vval = uv.g - 0.5;

    // BT.709 YUV to RGB conversion
    var rgb : vec3f;
    rgb.r = y + 1.5748 * vval;
    rgb.g = y - 0.1873 * uval - 0.4681 * vval;
    rgb.b = y + 1.8556 * uval;

    return vec4f(clamp(rgb, vec3f(0.0), vec3f(1.0)), 1.0);
}
