// Little Planet NV12 Shader (Stereographic projection from south pole)
// BT.709 color space conversion with stereographic mapping

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

// Convert screen UV to equirectangular UV using stereographic projection (south pole)
fn toLittlePlanet(uv: vec2f, yaw: f32, pitch: f32, zoom: f32, aspect: f32) -> vec2f {
    // Center and apply aspect ratio
    var x = (uv.x - 0.5) * 2.0 * aspect;
    var y = (uv.y - 0.5) * 2.0;

    // Apply zoom
    x = x / zoom;
    y = y / zoom;

    // Stereographic projection from south pole (r = tan(theta/2))
    let r = sqrt(x * x + y * y);

    // Discard pixels outside projection circle
    if (r > 3.14159265) {
        return vec2f(-1.0, -1.0); // Out of bounds
    }

    // Convert to spherical coordinates
    let theta = 2.0 * atan(r); // Latitude from south pole
    let phi = atan2(y, x);     // Longitude

    // Convert to 3D direction vector
    let sinTheta = sin(theta);
    let cosTheta = cos(theta);
    let sinPhi = sin(phi);
    let cosPhi = cos(phi);

    var dir = vec3f(sinTheta * cosPhi, sinTheta * sinPhi, -cosTheta);

    // Apply rotation (yaw and pitch)
    let cy = cos(yaw);
    let sy = sin(yaw);
    let cx = cos(pitch);
    let sx = sin(pitch);

    // Pitch rotation (around X axis)
    var rx = vec3f(dir.x, dir.y * cx - dir.z * sx, dir.y * sx + dir.z * cx);

    // Yaw rotation (around Y axis)
    var r3d = vec3f(rx.x * cy + rx.z * sy, rx.y, -rx.x * sy + rx.z * cy);

    // Convert back to equirectangular UV
    let lon = atan2(r3d.x, -r3d.z);
    let lat = asin(clamp(r3d.y, -1.0, 1.0));

    var uout = lon / (2.0 * 3.14159265) + 0.5;
    var vout = 0.5 - lat / 3.14159265;

    return vec2f(fract(uout), clamp(1.0 - vout, 0.0, 1.0));
}

@fragment
fn fs(input : VertexOutput) -> @location(0) vec4f {
    let yaw = u.x;
    let pitch = u.y;
    let zoom = max(u.z, 0.1);
    let aspect = max(u.w, 1.0);

    let planetUV = toLittlePlanet(input.uv, yaw, pitch, zoom, aspect);

    // Sample NV12 textures (must be before any non-uniform control flow)
    let y = textureSample(yTex, mySampler, planetUV).r;
    let uv_chroma = textureSample(uvTex, mySampler, planetUV).rg;
    let uval = uv_chroma.r - 0.5;
    let vval = uv_chroma.g - 0.5;

    // BT.709 YUV to RGB conversion
    var rgb : vec3f;
    rgb.r = y + 1.5748 * vval;
    rgb.g = y - 0.1873 * uval - 0.4681 * vval;
    rgb.b = y + 1.8556 * uval;

    // Check if out of bounds - use select to avoid non-uniform control flow
    let finalColor = select(rgb, vec3f(0.0), planetUV.x < 0.0);

    return vec4f(clamp(finalColor, vec3f(0.0), vec3f(1.0)), 1.0);
}
