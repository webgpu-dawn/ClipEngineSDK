#pragma once

#include <webgpu/webgpu_cpp.h>

#include <array>
#include <initializer_list>
#include <string>
#include <vector>

#include "Constants.h"

namespace dawn::utils {

wgpu::ShaderModule CreateShaderModule(const wgpu::Device& device, const char* source);

wgpu::Buffer CreateBufferFromData(const wgpu::Device& device,
                                  const void* data,
                                  uint64_t size,
                                  wgpu::BufferUsage usage);

struct ComboRenderPassDescriptor : public wgpu::RenderPassDescriptor {
public:
    ComboRenderPassDescriptor(const std::vector<wgpu::TextureView>& colorAttachmentInfo = {},
                              wgpu::TextureView depthStencil = wgpu::TextureView());
    ~ComboRenderPassDescriptor();

    ComboRenderPassDescriptor(const ComboRenderPassDescriptor& otherRenderPass);
    const ComboRenderPassDescriptor& operator=(const ComboRenderPassDescriptor& otherRenderPass);

    void UnsetDepthStencilLoadStoreOpsForFormat(wgpu::TextureFormat format);

    std::array<wgpu::RenderPassColorAttachment, kMaxColorAttachments> cColorAttachments;
    wgpu::RenderPassDepthStencilAttachment cDepthStencilAttachmentInfo = {};
};

}