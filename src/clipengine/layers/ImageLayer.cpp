#include "ImageLayer.h"
#include <iostream>

ImageLayer::ImageLayer(const ShaderConfig& config)
    : shaderConfig_(config) {
}

ImageLayer::~ImageLayer() = default;

bool ImageLayer::initialize(wgpu::Device device, wgpu::TextureFormat format) {
    device_ = device;
    surfaceFormat_ = format;

    initializeBuffers();
    initializeSampler();
    initializeShader();
    initializePipeline();

    // Initialize filter chain (will be used if filters are added)
    // We'll create filter output texture when needed (in setViewport or first render)
    filterChain_.initialize(device_, format, 1920, 1080);  // Default size, will be resized

    return true;
}

void ImageLayer::initializeBuffers() {
    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = sizeof(float) * shaderConfig_.vertexStride / sizeof(float) * 6;  // 6 vertices
    bufferDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    vertexBuffer_ = device_.CreateBuffer(&bufferDesc);

    updateVertexBuffer();
}

void ImageLayer::initializeSampler() {
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

void ImageLayer::initializeShader() {
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

void ImageLayer::initializePipeline() {
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

    // Vertex state - convert shader config to WebGPU format
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
        .format = surfaceFormat_,
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

void ImageLayer::updateTextures(const std::vector<wgpu::TextureView>& textureViews) {
    textureViews_ = textureViews;
    updateBindGroup();
}

void ImageLayer::updateBindGroup() {
    if (textureViews_.empty() && !shaderConfig_.bindings.empty()) {
        // No textures yet, but shader expects them
        return;
    }

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
                if (textureIndex < textureViews_.size()) {
                    entry.textureView = textureViews_[textureIndex++];
                } else {
                    // Not enough textures provided
                    return;
                }
                break;
                case ShaderBindingDesc::Type::Buffer: {
                    // Ensure we have a uniform buffer to bind
                    if (!uniformBuffer_) {
                        uint64_t size = binding.minBindingSize ? binding.minBindingSize : 16;
                        wgpu::BufferDescriptor bufDesc = {};
                        bufDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
                        bufDesc.size = size;
                        uniformBufferSize_ = size;
                        uniformBuffer_ = device_.CreateBuffer(&bufDesc);
                    }

                    if (!uniformBuffer_) {
                        // Could not create buffer — fail bind group creation
                        return;
                    }

                    entry.buffer = uniformBuffer_;
                    entry.offset = 0;
                    entry.size = uniformBufferSize_;
                    break;
                }
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

void ImageLayer::updateUniformData(const void* data, size_t size) {
    if (size == 0 || !data) return;
    if (!uniformBuffer_ || size > uniformBufferSize_) {
        // (re)create uniform buffer
        if (uniformBuffer_) uniformBuffer_ = nullptr;
        uniformBufferSize_ = (uint64_t)size;
        wgpu::BufferDescriptor bufDesc = {};
        bufDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        bufDesc.size = uniformBufferSize_;
        uniformBuffer_ = device_.CreateBuffer(&bufDesc);
    }

    device_.GetQueue().WriteBuffer(uniformBuffer_, 0, data, size);

    // Recreate bind group so buffer binding is included if shader expects it
    updateBindGroup();
}

void ImageLayer::render(wgpu::RenderPassEncoder& pass) {
    if (!enabled_ || !bindGroup_ || !pipeline_) return;

    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertexBuffer_);
    pass.SetBindGroup(0, bindGroup_);
    pass.Draw(6);
}

void ImageLayer::update(float deltaTime) {
    // Can be overridden by subclasses for animations
}

void ImageLayer::updateVertexBuffer() {
    // Convert transform (0-1 normalized) to NDC (-1 to 1)
    float x1 = transform_.x * 2.0f - 1.0f;
    float y1 = transform_.y * 2.0f - 1.0f;
    float x2 = (transform_.x + transform_.width) * 2.0f - 1.0f;
    float y2 = (transform_.y + transform_.height) * 2.0f - 1.0f;

    float vertices[] = {
        // pos.x, pos.y, uv.x, uv.y
        x1, y1, 0.0f, 1.0f,  // bottom-left
        x2, y1, 1.0f, 1.0f,  // bottom-right
        x1, y2, 0.0f, 0.0f,  // top-left
        x1, y2, 0.0f, 0.0f,  // top-left
        x2, y1, 1.0f, 1.0f,  // bottom-right
        x2, y2, 1.0f, 0.0f   // top-right
    };

    device_.GetQueue().WriteBuffer(vertexBuffer_, 0, vertices, sizeof(vertices));
}

void ImageLayer::setTransform(float x, float y, float width, float height) {
    transform_.x = x;
    transform_.y = y;
    transform_.width = width;
    transform_.height = height;

    if (vertexBuffer_) {
        updateVertexBuffer();
    }
}
