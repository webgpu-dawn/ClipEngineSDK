#include "WGPUHelpers.h"

namespace dawn::utils {

wgpu::ShaderModule CreateShaderModule(const wgpu::Device& device, const char* source) {
    wgpu::ShaderSourceWGSL wgsl;
    wgsl.code = source;
    wgpu::ShaderModuleDescriptor desc;
    desc.nextInChain = &wgsl;
    return device.CreateShaderModule(&desc);
}

wgpu::Buffer CreateBufferFromData(const wgpu::Device& device,
                                  const void* data,
                                  uint64_t size,
                                  wgpu::BufferUsage usage) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = size;
    descriptor.usage = usage | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    device.GetQueue().WriteBuffer(buffer, 0, data, size);
    return buffer;
}

ComboRenderPassDescriptor::ComboRenderPassDescriptor(
    const std::vector<wgpu::TextureView>& colorAttachmentInfo,
    wgpu::TextureView depthStencil) {
    for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
        cColorAttachments[i].loadOp = wgpu::LoadOp::Clear;
        cColorAttachments[i].storeOp = wgpu::StoreOp::Store;
        cColorAttachments[i].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
    }

    cDepthStencilAttachmentInfo.depthClearValue = 1.0f;
    cDepthStencilAttachmentInfo.stencilClearValue = 0;
    cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Clear;
    cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
    cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Clear;
    cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Store;

    colorAttachmentCount = colorAttachmentInfo.size();
    uint32_t colorAttachmentIndex = 0;
    for (const wgpu::TextureView& colorAttachment : colorAttachmentInfo) {
        if (colorAttachment.Get() != nullptr) {
            cColorAttachments[colorAttachmentIndex].view = colorAttachment;
        }
        ++colorAttachmentIndex;
    }
    colorAttachments = cColorAttachments.data();

    if (depthStencil.Get() != nullptr) {
        cDepthStencilAttachmentInfo.view = depthStencil;
        depthStencilAttachment = &cDepthStencilAttachmentInfo;
    } else {
        depthStencilAttachment = nullptr;
    }
}

ComboRenderPassDescriptor::~ComboRenderPassDescriptor() = default;

ComboRenderPassDescriptor::ComboRenderPassDescriptor(const ComboRenderPassDescriptor& other) {
    *this = other;
}

const ComboRenderPassDescriptor& ComboRenderPassDescriptor::operator=(
    const ComboRenderPassDescriptor& otherRenderPass) {
    cDepthStencilAttachmentInfo = otherRenderPass.cDepthStencilAttachmentInfo;
    cColorAttachments = otherRenderPass.cColorAttachments;
    colorAttachmentCount = otherRenderPass.colorAttachmentCount;

    colorAttachments = cColorAttachments.data();

    if (otherRenderPass.depthStencilAttachment != nullptr) {
        // Assign desc.depthStencilAttachment to this->depthStencilAttachmentInfo;
        depthStencilAttachment = &cDepthStencilAttachmentInfo;
    } else {
        depthStencilAttachment = nullptr;
    }

    return *this;
}
void ComboRenderPassDescriptor::UnsetDepthStencilLoadStoreOpsForFormat(wgpu::TextureFormat format) {
    switch (format) {
        case wgpu::TextureFormat::Depth24Plus:
        case wgpu::TextureFormat::Depth32Float:
        case wgpu::TextureFormat::Depth16Unorm:
            cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Undefined;
            cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Undefined;
            break;
        case wgpu::TextureFormat::Stencil8:
            cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Undefined;
            cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Undefined;
            break;
        default:
            break;
    }
}


}