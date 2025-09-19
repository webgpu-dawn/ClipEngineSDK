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

private:
    void init_buffer();
    void init_sampler();
    void init_texture();
    void init_shader();
    void init_bindgroup();
    void init_pipeline();

    void init_compute_pipeline_and_bind_group();

private:
    // 顶点缓冲 & 采样器
    wgpu::Buffer vertex_buffer_;
    wgpu::Sampler sampler_;

    wgpu::Buffer uniform_buffer_;

    // 渲染相关
    wgpu::BindGroupLayout bind_group_layout_;
    wgpu::BindGroup bind_group_;
    wgpu::RenderPipeline pipeline_;
    wgpu::ShaderModule module_;

    wgpu::TextureView y_tex_;
    wgpu::TextureView u_tex_;
    wgpu::TextureView v_tex_;

    wgpu::TextureView rgba_tex_;

    // ----------------------------
    // Compute Shader 相关
    // ----------------------------
    wgpu::ComputePipeline compute_pipeline_;
    wgpu::BindGroup computeBindGroup_;

    
};
