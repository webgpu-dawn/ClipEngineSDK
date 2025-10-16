#pragma once

#include <webgpu/webgpu_cpp.h>
#include <string>
#include <vector>

namespace RHI {

// ============================================================================
// 简单实用的资源创建辅助函数
// ============================================================================

class Helper {
public:
    explicit Helper(wgpu::Device device) : device_(device) {}

    // ========== 缓冲区 ==========

    // 创建顶点缓冲区（带初始数据）
    wgpu::Buffer createVertexBuffer(const void* data, size_t size, const char* label = nullptr) {
        wgpu::BufferDescriptor desc = {};
        desc.size = size;
        desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
        desc.label = label;

        auto buffer = device_.CreateBuffer(&desc);
        if (data) {
            device_.GetQueue().WriteBuffer(buffer, 0, data, size);
        }
        return buffer;
    }

    // 创建Uniform缓冲区
    wgpu::Buffer createUniformBuffer(size_t size, const char* label = nullptr) {
        wgpu::BufferDescriptor desc = {};
        desc.size = size;
        desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        desc.label = label;
        return device_.CreateBuffer(&desc);
    }

    // 创建索引缓冲区
    wgpu::Buffer createIndexBuffer(const void* data, size_t size, const char* label = nullptr) {
        wgpu::BufferDescriptor desc = {};
        desc.size = size;
        desc.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;
        desc.label = label;

        auto buffer = device_.CreateBuffer(&desc);
        if (data) {
            device_.GetQueue().WriteBuffer(buffer, 0, data, size);
        }
        return buffer;
    }

    // 通用缓冲区创建
    wgpu::Buffer createBuffer(size_t size, wgpu::BufferUsage usage, const void* data = nullptr, const char* label = nullptr) {
        wgpu::BufferDescriptor desc = {};
        desc.size = size;
        desc.usage = usage;
        desc.label = label;

        auto buffer = device_.CreateBuffer(&desc);
        if (data) {
            device_.GetQueue().WriteBuffer(buffer, 0, data, size);
        }
        return buffer;
    }

    // ========== 纹理 ==========

    // 创建2D纹理
    wgpu::Texture createTexture2D(
        uint32_t width,
        uint32_t height,
        wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm,
        wgpu::TextureUsage usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
        const char* label = nullptr
    ) {
        wgpu::TextureDescriptor desc = {};
        desc.size = {width, height, 1};
        desc.format = format;
        desc.usage = usage;
        desc.dimension = wgpu::TextureDimension::e2D;
        desc.mipLevelCount = 1;
        desc.sampleCount = 1;
        desc.label = label;
        return device_.CreateTexture(&desc);
    }

    // 创建渲染目标纹理
    wgpu::Texture createRenderTarget(
        uint32_t width,
        uint32_t height,
        wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm,
        uint32_t sampleCount = 1,
        const char* label = nullptr
    ) {
        wgpu::TextureDescriptor desc = {};
        desc.size = {width, height, 1};
        desc.format = format;
        desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
        desc.dimension = wgpu::TextureDimension::e2D;
        desc.mipLevelCount = 1;
        desc.sampleCount = sampleCount;
        desc.label = label;
        return device_.CreateTexture(&desc);
    }

    // ========== 采样器 ==========

    // 线性采样器（最常用）
    wgpu::Sampler createLinearSampler(const char* label = nullptr) {
        wgpu::SamplerDescriptor desc = {};
        desc.addressModeU = wgpu::AddressMode::ClampToEdge;
        desc.addressModeV = wgpu::AddressMode::ClampToEdge;
        desc.addressModeW = wgpu::AddressMode::ClampToEdge;
        desc.magFilter = wgpu::FilterMode::Linear;
        desc.minFilter = wgpu::FilterMode::Linear;
        desc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        desc.label = label;
        return device_.CreateSampler(&desc);
    }

    // 最近邻采样器
    wgpu::Sampler createNearestSampler(const char* label = nullptr) {
        wgpu::SamplerDescriptor desc = {};
        desc.addressModeU = wgpu::AddressMode::ClampToEdge;
        desc.addressModeV = wgpu::AddressMode::ClampToEdge;
        desc.addressModeW = wgpu::AddressMode::ClampToEdge;
        desc.magFilter = wgpu::FilterMode::Nearest;
        desc.minFilter = wgpu::FilterMode::Nearest;
        desc.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
        desc.label = label;
        return device_.CreateSampler(&desc);
    }

    // 重复采样器
    wgpu::Sampler createRepeatSampler(const char* label = nullptr) {
        wgpu::SamplerDescriptor desc = {};
        desc.addressModeU = wgpu::AddressMode::Repeat;
        desc.addressModeV = wgpu::AddressMode::Repeat;
        desc.addressModeW = wgpu::AddressMode::Repeat;
        desc.magFilter = wgpu::FilterMode::Linear;
        desc.minFilter = wgpu::FilterMode::Linear;
        desc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        desc.label = label;
        return device_.CreateSampler(&desc);
    }

    // ========== 着色器 ==========

    // 从WGSL代码创建着色器
    wgpu::ShaderModule createShader(const char* code, const char* label = nullptr) {
        wgpu::ShaderModuleWGSLDescriptor wgslDesc = {};
        wgslDesc.code = code;

        wgpu::ShaderModuleDescriptor desc = {};
        desc.nextInChain = &wgslDesc;
        desc.label = label;

        return device_.CreateShaderModule(&desc);
    }

    // 从文件加载着色器
    wgpu::ShaderModule createShaderFromFile(const char* path);

    // ========== 绑定组布局 ==========

    // 创建简单的绑定组布局（Uniform + Texture + Sampler）
    wgpu::BindGroupLayout createSimpleBindGroupLayout(const char* label = nullptr) {
        wgpu::BindGroupLayoutEntry entries[3] = {};

        // Uniform buffer
        entries[0].binding = 0;
        entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

        // Texture
        entries[1].binding = 1;
        entries[1].visibility = wgpu::ShaderStage::Fragment;
        entries[1].texture.sampleType = wgpu::TextureSampleType::Float;
        entries[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;

        // Sampler
        entries[2].binding = 2;
        entries[2].visibility = wgpu::ShaderStage::Fragment;
        entries[2].sampler.type = wgpu::SamplerBindingType::Filtering;

        wgpu::BindGroupLayoutDescriptor desc = {};
        desc.entryCount = 3;
        desc.entries = entries;
        desc.label = label;

        return device_.CreateBindGroupLayout(&desc);
    }

    // 通用绑定组布局创建
    wgpu::BindGroupLayout createBindGroupLayout(
        const std::vector<wgpu::BindGroupLayoutEntry>& entries,
        const char* label = nullptr
    ) {
        wgpu::BindGroupLayoutDescriptor desc = {};
        desc.entryCount = static_cast<uint32_t>(entries.size());
        desc.entries = entries.data();
        desc.label = label;
        return device_.CreateBindGroupLayout(&desc);
    }

    // ========== 绑定组 ==========

    wgpu::BindGroup createBindGroup(
        wgpu::BindGroupLayout layout,
        const std::vector<wgpu::BindGroupEntry>& entries,
        const char* label = nullptr
    ) {
        wgpu::BindGroupDescriptor desc = {};
        desc.layout = layout;
        desc.entryCount = static_cast<uint32_t>(entries.size());
        desc.entries = entries.data();
        desc.label = label;
        return device_.CreateBindGroup(&desc);
    }

    // ========== 管线布局 ==========

    wgpu::PipelineLayout createPipelineLayout(
        const std::vector<wgpu::BindGroupLayout>& layouts,
        const char* label = nullptr
    ) {
        wgpu::PipelineLayoutDescriptor desc = {};
        desc.bindGroupLayoutCount = static_cast<uint32_t>(layouts.size());
        desc.bindGroupLayouts = layouts.data();
        desc.label = label;
        return device_.CreatePipelineLayout(&desc);
    }

    // ========== 获取设备 ==========

    wgpu::Device getDevice() const { return device_; }
    wgpu::Queue getQueue() const { return device_.GetQueue(); }

private:
    wgpu::Device device_;
};

// ============================================================================
// 常用顶点布局
// ============================================================================

namespace VertexLayout {

    // Position + UV (vec2 + vec2)
    inline wgpu::VertexBufferLayout PositionUV() {
        static wgpu::VertexAttribute attrs[2] = {
            {wgpu::VertexFormat::Float32x2, 0, 0},                    // position
            {wgpu::VertexFormat::Float32x2, sizeof(float) * 2, 1}     // uv
        };

        static wgpu::VertexBufferLayout layout = {};
        layout.arrayStride = sizeof(float) * 4;
        layout.stepMode = wgpu::VertexStepMode::Vertex;
        layout.attributeCount = 2;
        layout.attributes = attrs;

        return layout;
    }

    // Position 3D + Normal + UV (vec3 + vec3 + vec2)
    inline wgpu::VertexBufferLayout PositionNormalUV() {
        static wgpu::VertexAttribute attrs[3] = {
            {wgpu::VertexFormat::Float32x3, 0, 0},                    // position
            {wgpu::VertexFormat::Float32x3, sizeof(float) * 3, 1},    // normal
            {wgpu::VertexFormat::Float32x2, sizeof(float) * 6, 2}     // uv
        };

        static wgpu::VertexBufferLayout layout = {};
        layout.arrayStride = sizeof(float) * 8;
        layout.stepMode = wgpu::VertexStepMode::Vertex;
        layout.attributeCount = 3;
        layout.attributes = attrs;

        return layout;
    }

} // namespace VertexLayout

} // namespace RHI
