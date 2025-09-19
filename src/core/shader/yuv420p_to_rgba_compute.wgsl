@group(0) @binding(0) var y_tex: texture_2d<f32>;
@group(0) @binding(1) var u_tex: texture_2d<f32>;
@group(0) @binding(2) var v_tex: texture_2d<f32>;
@group(0) @binding(3) var out_tex: texture_storage_2d<rgba8unorm, write>;

@compute @workgroup_size(16, 16)

fn main(@builtin(global_invocation_id) gid: vec3u) {
    let dims = textureDimensions(y_tex);
    if gid.x >= dims.x || gid.y >= dims.y { return; }

    // --- 采样 Y 值 ---
    let y = textureLoad(y_tex, vec2i(gid.xy), 0).r;

    // --- 采样 UV 值（注意下采样 2x2）---
    let uvCoord = vec2i(gid.xy) / 2;
    let u = textureLoad(u_tex, uvCoord, 0).r;
    let v = textureLoad(v_tex, uvCoord, 0).r;

    // --- YUV -> RGB (BT.601 full range) ---
    let Y = y * 255.0;
    let U = u * 255.0 - 128.0;
    let V = v * 255.0 - 128.0;

    var R = Y + 1.402 * V;
    var G = Y - 0.344136 * U - 0.714136 * V;
    var B = Y + 1.772 * U;

    // 归一化回 [0,1]
    R = clamp(R / 255.0, 0.0, 1.0);
    G = clamp(G / 255.0, 0.0, 1.0);
    B = clamp(B / 255.0, 0.0, 1.0);

    textureStore(out_tex, vec2i(gid.xy), vec4f(R, G, B, 1.0));
            // textureStore(out_tex, vec2i(gid.xy), vec4f(1.0, 0.0, 0.0, 1.0));
}