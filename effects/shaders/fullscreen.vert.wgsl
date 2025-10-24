// Fullscreen quad vertex shader
// Used by most post-processing effects

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
