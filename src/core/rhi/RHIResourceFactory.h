#pragma once

#include <webgpu/webgpu_cpp.h>
#include <initializer_list>
#include <fstream>
#include <sstream>
#include <filesystem>

class RHIResourceFactory {
public:
    explicit RHIResourceFactory(wgpu::Device device) : device_(device) {}

    // Create sampler
    wgpu::Sampler createSampler();

    // Create buffer
    wgpu::Buffer createBuffer(const void* data, uint64_t size, wgpu::BufferUsage usage);

    // Create shader from file path or code
    wgpu::ShaderModule createShaderFromPath(const char* path);
    wgpu::ShaderModule createShaderFromCode(const char* code);

    // Create texture from file path
    wgpu::Texture createTextureFromPath(const char* path);

    // Create bind group layout and bind group
    wgpu::BindGroupLayout createBindGroupLayout(std::initializer_list<wgpu::BindGroupLayoutEntry> entries);
    wgpu::BindGroup createBindGroup(wgpu::BindGroupLayout& layout, std::initializer_list<wgpu::BindGroupEntry> entries);

    // Create render pipeline
    wgpu::RenderPipeline createRenderPipeline();

private:
    wgpu::Device device_;
};
