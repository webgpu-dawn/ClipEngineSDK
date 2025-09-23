#pragma once
#include "IRenderable.h"
#include <vector>

class EquirectangularRenderer : public IRenderable {
public:
    EquirectangularRenderer(wgpu::Device device, wgpu::TextureFormat format)
        : IRenderable(device, format) {}

    void init() override;
    void render(wgpu::RenderPassEncoder& pass) override;

private:
    void init_buffer();
    void init_sampler();
    void init_texture();
    void init_shader();
    void init_pipeline();
    void init_bindgroup();

private:
    wgpu::Buffer vertex_buffer_;
    wgpu::Buffer index_buffer_;
    uint32_t index_count_;

    wgpu::Sampler sampler_;
    wgpu::TextureView rgba_tex_;
    wgpu::ShaderModule module_;
    wgpu::RenderPipeline pipeline_;
    wgpu::BindGroupLayout bind_group_layout_;
    wgpu::BindGroup bind_group_;
    wgpu::Buffer uniform_buffer_;
};
