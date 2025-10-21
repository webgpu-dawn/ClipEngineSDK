#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../core/CeRenderable.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

// Forward declarations (GPUTimer is in global namespace)
class GPUProfiler;
class GPUTimer;

enum class VideoFormat {
    NV12,
    I420,
    RGBA
};

class VideoRenderer : public CeRenderable {
public:
    VideoRenderer();
    ~VideoRenderer() override;

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void render(wgpu::RenderPassEncoder& pass) override;
    void update(float deltaTime) override;
    CeRendererType getType() const override { return CeRendererType::Video; }
    void setViewport(float x, float y, float width, float height) override;

    bool updateFrame(ID3D11Texture2D* texture, int arrayIndex = 0);

    void setVideoFormat(VideoFormat format) { videoFormat_ = format; }

    // 性能分析
    void setProfiler(std::shared_ptr<GPUProfiler> profiler) { profiler_ = profiler; }
    std::shared_ptr<GPUProfiler> getProfiler() const { return profiler_; }

    // GPU计时器
    void setGPUTimer(std::shared_ptr<GPUTimer> timer) { gpuTimer_ = timer; }
    std::shared_ptr<GPUTimer> getGPUTimer() const { return gpuTimer_; }

    enum class FillMode {
        Fit,
        Fill,
        Stretch
    };
    void setFillMode(FillMode mode) { fillMode_ = mode; }

private:
    void initializeBuffers();
    void initializeSampler();
    void initializeShader();
    void initializePipeline();
    void updateBindGroup();
    void updateVertexBuffer();

    wgpu::Buffer vertexBuffer_;
    wgpu::Sampler sampler_;
    wgpu::ShaderModule shaderModule_;
    wgpu::RenderPipeline pipeline_;
    wgpu::BindGroup bindGroup_;
    wgpu::BindGroupLayout bindGroupLayout_;

    wgpu::TextureView yPlaneView_;
    wgpu::TextureView uvPlaneView_;

    VideoFormat videoFormat_ = VideoFormat::NV12;
    FillMode fillMode_ = FillMode::Fit;

    std::shared_ptr<GPUProfiler> profiler_;
    std::shared_ptr<GPUTimer> gpuTimer_;
};
