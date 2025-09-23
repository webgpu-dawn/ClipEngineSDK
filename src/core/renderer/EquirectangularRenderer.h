#pragma once

#include "IRenderable.h"

class EquirectangularRenderer : public IRenderable 
{
public:
    EquirectangularRenderer(wgpu::Device device, wgpu::TextureFormat format)
        : IRenderable(device, format) { }

    void init();
    void render(wgpu::RenderPassEncoder& pass);

private:
    void init_buffer();
    void init_sampler();
    void init_texture();
    void init_shader();
    void init_bindgroup();
    void init_pipeline();

private:
    wgpu::Buffer vertex_buffer_;
    wgpu::Sampler sampler_;

    wgpu::Buffer uniform_buffer_;

    // 渲染相关
    

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