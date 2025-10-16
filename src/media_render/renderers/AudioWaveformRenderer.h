#pragma once
#include "../core/MediaRenderer.h"
#include <vector>

namespace MediaRender {

class AudioWaveformRenderer : public IMediaRenderer {
public:
    AudioWaveformRenderer();
    ~AudioWaveformRenderer() override = default;

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void render(wgpu::RenderPassEncoder& pass) override;
    void update(float deltaTime) override;
    RendererType getType() const override { return RendererType::Audio; }
    void setViewport(float x, float y, float width, float height) override;

    void updateAudioData(const float* samples, size_t sampleCount, int channels);

    void setWaveformColor(float r, float g, float b, float a = 1.0f);

    enum class DisplayMode {
        Waveform,
        Spectrum,
        Both
    };
    void setDisplayMode(DisplayMode mode) { displayMode_ = mode; }

private:
    void initializeShader();
    void initializePipeline();
    void updateVertexBuffer();

    wgpu::Buffer vertexBuffer_;
    wgpu::ShaderModule shaderModule_;
    wgpu::RenderPipeline pipeline_;

    std::vector<float> audioSamples_;
    float waveformColor_[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    DisplayMode displayMode_ = DisplayMode::Waveform;

    size_t maxSamples_ = 2048;
    int channels_ = 2;
};

} // namespace MediaRender
