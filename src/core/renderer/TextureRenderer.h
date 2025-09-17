#pragma once
#include "IRenderable.h"
#include <webgpu/webgpu_cpp.h>

class TextureRenderer : public IRenderable
{
public:
    TextureRenderer(wgpu::Device device, wgpu::TextureFormat format)
        : IRenderable(device, format) { }

    void init();
    void render(wgpu::RenderPassEncoder& pass) override;

    void setTexture(const char* path); // 绑定纹理

private:
    wgpu::Buffer vertex_buffer_;
    wgpu::Sampler sampler_;
    wgpu::BindGroupLayout bind_group_layout_;
    wgpu::BindGroup bind_group_;
    wgpu::RenderPipeline pipeline_;
};