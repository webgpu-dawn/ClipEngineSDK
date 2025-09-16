#pragma once

#include <webgpu/webgpu_cpp.h>

#include <array>
#include <vector>

#include "Constants.h"

namespace dawn::utils {

class ComboRenderPipelineDescriptor : public wgpu::RenderPipelineDescriptor {
public:
    ComboRenderPipelineDescriptor();

    ComboRenderPipelineDescriptor(const ComboRenderPipelineDescriptor&) = delete;
    ComboRenderPipelineDescriptor& operator=(const ComboRenderPipelineDescriptor&) = delete;
    ComboRenderPipelineDescriptor(ComboRenderPipelineDescriptor&&) = delete;
    ComboRenderPipelineDescriptor& operator=(ComboRenderPipelineDescriptor&&) = delete;

    wgpu::DepthStencilState* EnableDepthStencil(
        wgpu::TextureFormat format = wgpu::TextureFormat::Depth24PlusStencil8);
    void DisableDepthStencil();

    std::array<wgpu::VertexBufferLayout, kMaxVertexBuffers> cBuffers;
    std::array<wgpu::VertexAttribute, kMaxVertexAttributes> cAttributes;
    std::array<wgpu::ColorTargetState, kMaxColorAttachments> cTargets;
    std::array<wgpu::BlendState, kMaxColorAttachments> cBlends;

    wgpu::FragmentState cFragment;
    wgpu::DepthStencilState cDepthStencil;
};

}