#pragma once

#include "RHICommon.h"

class RHIResourceFactory
{
public:
    RHIResourceFactory() = default;
    ~RHIResourceFactory() = default;

    explicit RHIResourceFactory(const wgpu::Device& device) : device_(device) {}

    // Sampler
    wgpu::Sampler createSampler();
    
    // Buffer
    wgpu::Buffer createBuffer(const void* data, uint64_t size, wgpu::BufferUsage usage);
    
    // Texture  
    wgpu::Texture createTextureFromPath(const char* path);

    // Shader
    wgpu::ShaderModule createShaderFromPath(const char* path);
    wgpu::ShaderModule createShaderFromCode(const char* code);

    wgpu::BindGroupLayout createBindGroupLayout(
        std::initializer_list<wgpu::BindGroupLayoutEntry> entries);
    wgpu::BindGroup createBindGroup(
        wgpu::BindGroupLayout& layout,
        std::initializer_list<wgpu::BindGroupEntry> entries
    );

    wgpu::RenderPipeline createRenderPipeline();

private:
    wgpu::Device device_;

};