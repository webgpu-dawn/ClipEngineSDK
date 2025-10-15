#pragma once
#include "IRenderable.h"
#include <vector>

#include "../rhi/RHIResourceFactory.h"

class EquirectangularRenderer : public IRenderable {
public:
    EquirectangularRenderer(wgpu::Device device, wgpu::TextureFormat format): IRenderable(device, format) {
        res_factory_ = std::make_unique<RHIResourceFactory>(device);
    }

    void init() override;
    void render(wgpu::RenderPassEncoder& pass) override;

private:
    void init_buffer();
    void init_pipeline();
    void init_bindgroup();

private:
    wgpu::Buffer vertex_buffer_;
    wgpu::Buffer index_buffer_;
    uint32_t index_count_;

    wgpu::Sampler sampler_;
    wgpu::TextureView rgba_tex_;
    wgpu::ShaderModule shader_;
    wgpu::RenderPipeline pipeline_;
    wgpu::BindGroupLayout bind_group_layout_;
    wgpu::BindGroup bind_group_;
    wgpu::Buffer uniform_buffer_;

    std::unique_ptr<RHIResourceFactory> res_factory_;
};
