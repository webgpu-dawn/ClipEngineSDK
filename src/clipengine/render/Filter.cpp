#include "Filter.h"
#include <iostream>

void Filter::createFullScreenQuad() {
    // Full-screen quad in NDC (-1 to 1)
    float vertices[] = {
        // pos.x, pos.y, uv.x, uv.y
        -1.0f, -1.0f, 0.0f, 1.0f,  // bottom-left
         1.0f, -1.0f, 1.0f, 1.0f,  // bottom-right
        -1.0f,  1.0f, 0.0f, 0.0f,  // top-left
        -1.0f,  1.0f, 0.0f, 0.0f,  // top-left
         1.0f, -1.0f, 1.0f, 1.0f,  // bottom-right
         1.0f,  1.0f, 1.0f, 0.0f   // top-right
    };

    wgpu::BufferDescriptor bufferDesc = {
        .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
        .size = sizeof(vertices)
    };
    vertexBuffer_ = device_.CreateBuffer(&bufferDesc);
    device_.GetQueue().WriteBuffer(vertexBuffer_, 0, vertices, sizeof(vertices));
}

void Filter::createSampler() {
    wgpu::SamplerDescriptor samplerDesc = {
        .addressModeU = wgpu::AddressMode::ClampToEdge,
        .addressModeV = wgpu::AddressMode::ClampToEdge,
        .addressModeW = wgpu::AddressMode::ClampToEdge,
        .magFilter    = wgpu::FilterMode::Linear,
        .minFilter    = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear
    };
    sampler_ = device_.CreateSampler(&samplerDesc);
}

void Filter::updateUniformBuffer(const void* data, size_t size) {
    if (size == 0 || !data) return;

    if (!uniformBuffer_ || size > uniformBufferSize_) {
        uniformBufferSize_ = (uint64_t)size;
        wgpu::BufferDescriptor bufDesc = {
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = uniformBufferSize_
        };
        uniformBuffer_ = device_.CreateBuffer(&bufDesc);
    }

    device_.GetQueue().WriteBuffer(uniformBuffer_, 0, data, size);
}

// ============================================================================
// ShaderFilter Implementation
// ============================================================================

ShaderFilter::ShaderFilter(const std::string& name, const ShaderConfig& config)
    : name_(name), shaderConfig_(config) {
}

bool ShaderFilter::initialize(wgpu::Device device, wgpu::TextureFormat format) {
    device_ = device;
    outputFormat_ = format;

    createFullScreenQuad();
    createSampler();
    initializeShader();
    initializePipeline();

    return true;
}

void ShaderFilter::initializeShader() {
    // Create vertex shader module
    wgpu::ShaderModuleWGSLDescriptor vertexWgslDesc = {};
    vertexWgslDesc.code = shaderConfig_.vertexShaderSource.c_str();

    wgpu::ShaderModuleDescriptor vertexModuleDesc = {};
    vertexModuleDesc.nextInChain = &vertexWgslDesc;
    vertexShaderModule_ = device_.CreateShaderModule(&vertexModuleDesc);

    // Create fragment shader module
    wgpu::ShaderModuleWGSLDescriptor fragmentWgslDesc = {};
    fragmentWgslDesc.code = shaderConfig_.fragmentShaderSource.c_str();

    wgpu::ShaderModuleDescriptor fragmentModuleDesc = {};
    fragmentModuleDesc.nextInChain = &fragmentWgslDesc;
    fragmentShaderModule_ = device_.CreateShaderModule(&fragmentModuleDesc);
}

void ShaderFilter::initializePipeline() {
    // Create bind group layout from shader config
    std::vector<wgpu::BindGroupLayoutEntry> entries;
    for (const auto& binding : shaderConfig_.bindings) {
        wgpu::BindGroupLayoutEntry entry = {};
        entry.binding = binding.binding;
        entry.visibility = binding.visibility;

        switch (binding.type) {
            case ShaderBindingDesc::Type::Sampler:
                entry.sampler.type = binding.samplerType;
                break;
            case ShaderBindingDesc::Type::Texture:
                entry.texture.sampleType = binding.textureSampleType;
                entry.texture.viewDimension = binding.textureViewDimension;
                break;
            case ShaderBindingDesc::Type::Buffer:
                entry.buffer.type = binding.bufferType;
                entry.buffer.hasDynamicOffset = binding.hasDynamicOffset;
                entry.buffer.minBindingSize = binding.minBindingSize;
                break;
        }

        entries.push_back(entry);
    }

    wgpu::BindGroupLayoutDescriptor bglDesc = {
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.data()
    };
    bindGroupLayout_ = device_.CreateBindGroupLayout(&bglDesc);

    // Pipeline layout
    wgpu::PipelineLayoutDescriptor layoutDesc = {
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &bindGroupLayout_
    };
    wgpu::PipelineLayout pipelineLayout = device_.CreatePipelineLayout(&layoutDesc);

    // Vertex state
    std::vector<wgpu::VertexAttribute> attrs;
    for (const auto& attr : shaderConfig_.vertexAttributes) {
        wgpu::VertexAttribute wgpuAttr = {};
        wgpuAttr.format = attr.format;
        wgpuAttr.offset = attr.offset;
        wgpuAttr.shaderLocation = attr.shaderLocation;
        attrs.push_back(wgpuAttr);
    }

    wgpu::VertexBufferLayout vbLayout = {
        .arrayStride = shaderConfig_.vertexStride,
        .attributeCount = static_cast<uint32_t>(attrs.size()),
        .attributes = attrs.data()
    };

    // Fragment state
    wgpu::ColorTargetState colorTarget = {
        .format = outputFormat_,
        .writeMask = wgpu::ColorWriteMask::All
    };

    wgpu::FragmentState fragmentState = {
        .module = fragmentShaderModule_,
        .entryPoint = "fs",
        .targetCount = 1,
        .targets = &colorTarget
    };

    // Pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {
        .layout = pipelineLayout,
        .vertex = {
            .module = vertexShaderModule_,
            .entryPoint = "vs",
            .bufferCount = 1,
            .buffers = &vbLayout
        },
        .primitive = {
            .topology = shaderConfig_.topology
        },
        .fragment = &fragmentState
    };

    pipeline_ = device_.CreateRenderPipeline(&pipelineDesc);
}

void ShaderFilter::updateBindGroup() {
    std::vector<wgpu::BindGroupEntry> entries;

    size_t textureIndex = 0;
    for (const auto& binding : shaderConfig_.bindings) {
        wgpu::BindGroupEntry entry = {};
        entry.binding = binding.binding;

        switch (binding.type) {
            case ShaderBindingDesc::Type::Sampler:
                entry.sampler = sampler_;
                break;
            case ShaderBindingDesc::Type::Texture:
                if (textureIndex < currentInputs_.size()) {
                    entry.textureView = currentInputs_[textureIndex++];
                } else {
                    return; // Not enough textures
                }
                break;
            case ShaderBindingDesc::Type::Buffer:
                if (!uniformBuffer_) {
                    uint64_t size = binding.minBindingSize ? binding.minBindingSize : 16;
                    wgpu::BufferDescriptor bufDesc = {
                        .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
                        .size = size
                    };
                    uniformBufferSize_ = size;
                    uniformBuffer_ = device_.CreateBuffer(&bufDesc);
                }
                entry.buffer = uniformBuffer_;
                entry.offset = 0;
                entry.size = uniformBufferSize_;
                break;
        }

        entries.push_back(entry);
    }

    wgpu::BindGroupDescriptor bgDesc = {
        .layout = bindGroupLayout_,
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.data()
    };
    bindGroup_ = device_.CreateBindGroup(&bgDesc);
}

void ShaderFilter::apply(wgpu::RenderPassEncoder& pass, const std::vector<wgpu::TextureView>& inputTextures) {
    if (!enabled_) return;

    // Update input textures and bind group
    currentInputs_ = inputTextures;
    updateBindGroup();

    if (!bindGroup_) return;

    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertexBuffer_);
    pass.SetBindGroup(0, bindGroup_);
    pass.Draw(6);
}
