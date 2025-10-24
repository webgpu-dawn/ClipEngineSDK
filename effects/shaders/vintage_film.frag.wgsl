// Vintage Film Fragment Shader
// Creates old movie look with sepia tone, grain, scratches, and vignette

@group(0) @binding(0) var mySampler : sampler;
@group(0) @binding(1) var inputTexture : texture_2d<f32>;
@group(0) @binding(2) var<uniform> params : vec4f;  // grainIntensity, iTime, scratchIntensity, sepiaIntensity
@group(0) @binding(3) var<uniform> params2 : vec4f; // vignetteIntensity, unused, unused, unused

struct VertexOutput {
    @builtin(position) pos : vec4f,
    @location(0) uv : vec2f
};

// Pseudo-random function
fn random(st: vec2f) -> f32 {
    return fract(sin(dot(st, vec2f(12.9898, 78.233))) * 43758.5453123);
}

// Film grain noise
fn filmGrain(uv: vec2f, time: f32, intensity: f32) -> f32 {
    let noise = random(uv + vec2f(time * 0.0001, time * 0.0002));
    return (noise - 0.5) * intensity;
}

// Sepia tone conversion
fn applySepia(color: vec3f, intensity: f32) -> vec3f {
    let sepia = vec3f(
        dot(color, vec3f(0.393, 0.769, 0.189)),
        dot(color, vec3f(0.349, 0.686, 0.168)),
        dot(color, vec3f(0.272, 0.534, 0.131))
    );
    return mix(color, sepia, intensity);
}

// Vertical scratches
fn scratches(uv: vec2f, time: f32, intensity: f32) -> f32 {
    var scratch = 0.0;

    // Random vertical scratch positions
    let scratchX1 = fract(sin(floor(time * 0.5)) * 43758.5453);
    let scratchX2 = fract(sin(floor(time * 0.3 + 100.0)) * 43758.5453);

    // Create thin scratches
    let scratchWidth = 0.001;
    if abs(uv.x - scratchX1) < scratchWidth {
        scratch += random(vec2f(uv.y, time)) * intensity * 0.5;
    }
    if abs(uv.x - scratchX2) < scratchWidth {
        scratch += random(vec2f(uv.y + 100.0, time)) * intensity * 0.3;
    }

    return scratch;
}

@fragment
fn fs(input : VertexOutput) -> @location(0) vec4f {
    // Extract parameters
    let grainIntensity = params.x;
    let time = params.y;
    let scratchIntensity = params.z;
    let sepiaIntensity = params.w;
    let vignetteIntensity = params2.x;

    // Sample original color
    var color = textureSample(inputTexture, mySampler, input.uv);

    // Apply sepia tone
    color = vec4f(applySepia(color.rgb, sepiaIntensity), color.a);

    // Add film grain
    let grain = filmGrain(input.uv, time, grainIntensity);
    color = vec4f(clamp(color.rgb + vec3f(grain), vec3f(0.0), vec3f(1.0)), color.a);

    // Add scratches
    let scratchEffect = scratches(input.uv, time, scratchIntensity);
    color = vec4f(color.rgb + vec3f(scratchEffect), color.a);

    // Apply vignette
    let center = vec2f(0.5, 0.5);
    let dist = distance(input.uv, center);
    let vignette = smoothstep(0.8, 0.4, dist);
    let vignetteFactor = mix(1.0 - vignetteIntensity * 0.7, 1.0, vignette);
    color = vec4f(color.rgb * vignetteFactor, color.a);

    // Slight contrast boost for vintage look
    color = vec4f(pow(color.rgb, vec3f(1.1)), color.a);

    return clamp(color, vec4f(0.0), vec4f(1.0));
}
