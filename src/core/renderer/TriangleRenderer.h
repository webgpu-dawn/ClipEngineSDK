#pragma once

#include "IRenderable.h"

class TriangleRenderer : public IRenderable
{
public:
    TriangleRenderer(
        wgpu::Device device,
        wgpu::TextureFormat format
    ) : IRenderable(device, format) { }

    void init();
    void render(wgpu::RenderPassEncoder& pass);

private:
    wgpu::Buffer vertex_buffer_;
    wgpu::ShaderModule module_;
};