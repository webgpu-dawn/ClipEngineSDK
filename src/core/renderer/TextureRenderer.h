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

    void setTexture(const char* path); // 绑定纹理并生成处理纹理

private:
    // 顶点缓冲 & 采样器
    wgpu::Buffer vertex_buffer_;
    wgpu::Sampler sampler_;

    // 渲染相关
    wgpu::BindGroupLayout bind_group_layout_;
    wgpu::BindGroup bind_group_;
    wgpu::RenderPipeline pipeline_;

    // ----------------------------
    // Compute Shader 相关
    // ----------------------------
    wgpu::ComputePipeline computePipeline_;
    wgpu::BindGroup computeBindGroup_;

    // 原始纹理
    wgpu::TextureView originalTexture_;

    // 处理后的纹理
    wgpu::Texture processedTexture_;
    wgpu::TextureView processedTextureView_;
};
