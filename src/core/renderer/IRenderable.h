#pragma once

#include <webgpu/webgpu_cpp.h>
#include "../util/WGPUHelpers.h"
#include "../util/ComboRenderPipelineDescriptor.h"

class IRenderable
{
public:
    IRenderable(
        wgpu::Device device, 
        wgpu::TextureFormat format
    ) : device_(device), surface_texture_fmt_(format) { }
    
    virtual ~IRenderable() = default;
    virtual void render(wgpu::RenderPassEncoder& pass) = 0;

protected:
    wgpu::Device device_;
    wgpu::TextureFormat surface_texture_fmt_ = wgpu::TextureFormat::BGRA8Unorm;
    wgpu::RenderPipeline pipeline_;
    wgpu::ShaderModule module_;
};