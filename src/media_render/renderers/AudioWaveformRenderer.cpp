#include "AudioWaveformRenderer.h"
#include <algorithm>
#include <cmath>

namespace MediaRender {

AudioWaveformRenderer::AudioWaveformRenderer() {
    audioSamples_.resize(maxSamples_, 0.0f);
}

bool AudioWaveformRenderer::initialize(wgpu::Device device, wgpu::TextureFormat format) {
    device_ = device;
    surfaceFormat_ = format;

    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = maxSamples_ * sizeof(float) * 2;
    bufferDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    vertexBuffer_ = device_.CreateBuffer(&bufferDesc);

    initializeShader();
    initializePipeline();

    return true;
}

void AudioWaveformRenderer::initializeShader() {
    const char* shaderSource = R"(
        struct VertexInput {
            @location(0) position : vec2f
        };

        struct VertexOutput {
            @builtin(position) position : vec4f,
            @location(0) color : vec4f
        };

        @group(0) @binding(0) var<uniform> waveformColor : vec4f;

        @vertex
        fn vs(input : VertexInput) -> VertexOutput {
            var output : VertexOutput;
            output.position = vec4f(input.position, 0.0, 1.0);
            output.color = waveformColor;
            return output;
        }

        @fragment
        fn fs(input : VertexOutput) -> @location(0) vec4f {
            return input.color;
        }
    )";

    wgpu::ShaderModuleWGSLDescriptor wgslDesc = {};
    wgslDesc.code = shaderSource;

    wgpu::ShaderModuleDescriptor moduleDesc = {};
    moduleDesc.nextInChain = &wgslDesc;
    shaderModule_ = device_.CreateShaderModule(&moduleDesc);
}

void AudioWaveformRenderer::initializePipeline() {
    // Vertex attributes
    wgpu::VertexAttribute attrs[1] = {};
    attrs[0].format = wgpu::VertexFormat::Float32x2;
    attrs[0].offset = 0;
    attrs[0].shaderLocation = 0;

    wgpu::VertexBufferLayout vbLayout = {};
    vbLayout.arrayStride = sizeof(float) * 2;
    vbLayout.attributeCount = 1;
    vbLayout.attributes = attrs;

    // Fragment state
    wgpu::ColorTargetState colorTarget = {};
    colorTarget.format = surfaceFormat_;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;
    colorTarget.blend = nullptr; // No blending for now

    wgpu::FragmentState fragmentState = {};
    fragmentState.module = shaderModule_;
    fragmentState.entryPoint = "fs";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    // Pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.vertex.module = shaderModule_;
    pipelineDesc.vertex.entryPoint = "vs";
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vbLayout;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::LineStrip;

    pipeline_ = device_.CreateRenderPipeline(&pipelineDesc);
}

void AudioWaveformRenderer::updateAudioData(const float* samples, size_t sampleCount, int channels) {
    channels_ = channels;

    size_t samplesPerPoint = std::max<size_t>(1, sampleCount / maxSamples_);

    audioSamples_.clear();
    for(size_t i = 0; i < sampleCount; i += samplesPerPoint) {
        if(audioSamples_.size() >= maxSamples_) break;

        float value = 0.0f;
        for(size_t j = 0; j < samplesPerPoint && (i + j) < sampleCount; ++j) {
            value += samples[i + j];
        }
        value /= samplesPerPoint;

        audioSamples_.push_back(std::clamp(value, -1.0f, 1.0f));
    }

    updateVertexBuffer();
}

void AudioWaveformRenderer::updateVertexBuffer() {
    if(audioSamples_.empty()) return;

    std::vector<float> vertices;
    vertices.reserve(audioSamples_.size() * 2);

    for(size_t i = 0; i < audioSamples_.size(); ++i) {
        float x = viewport_.x + (float)i / (float)audioSamples_.size() * viewport_.width;
        float y = viewport_.y + audioSamples_[i] * viewport_.height * 0.5f;

        x = x * 2.0f - 1.0f;
        y = y * 2.0f - 1.0f;

        vertices.push_back(x);
        vertices.push_back(y);
    }

    device_.GetQueue().WriteBuffer(vertexBuffer_, 0, vertices.data(), vertices.size() * sizeof(float));
}

void AudioWaveformRenderer::render(wgpu::RenderPassEncoder& pass) {
    if(!enabled_ || audioSamples_.empty()) return;

    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertexBuffer_);
    pass.Draw(static_cast<uint32_t>(audioSamples_.size()));
}

void AudioWaveformRenderer::update(float deltaTime) {
}

void AudioWaveformRenderer::setViewport(float x, float y, float width, float height) {
    viewport_.x = x;
    viewport_.y = y;
    viewport_.width = width;
    viewport_.height = height;
    updateVertexBuffer();
}

void AudioWaveformRenderer::setWaveformColor(float r, float g, float b, float a) {
    waveformColor_[0] = r;
    waveformColor_[1] = g;
    waveformColor_[2] = b;
    waveformColor_[3] = a;
}

} // namespace MediaRender
