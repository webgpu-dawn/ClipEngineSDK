#pragma once

#include "../utils/Common.h"
#include <string>
#include <vector>

/**
 * @brief Describes a bind group entry in the shader
 */
struct ShaderBindingDesc {
    uint32_t binding;
    wgpu::ShaderStage visibility;

    // Only one of these should be set
    enum class Type {
        Sampler,
        Texture,
        Buffer
    } type;

    // For sampler
    wgpu::SamplerBindingType samplerType = wgpu::SamplerBindingType::Filtering;

    // For texture
    wgpu::TextureSampleType textureSampleType = wgpu::TextureSampleType::Float;
    wgpu::TextureViewDimension textureViewDimension = wgpu::TextureViewDimension::e2D;

    // For buffer
    wgpu::BufferBindingType bufferType = wgpu::BufferBindingType::Uniform;
    bool hasDynamicOffset = false;
    uint64_t minBindingSize = 0;
};

/**
 * @brief Configuration for a shader-based renderer
 */
struct ShaderConfig {
    std::string name;
    std::string vertexShaderSource;
    std::string fragmentShaderSource;

    std::vector<ShaderBindingDesc> bindings;

    // Vertex layout
    struct VertexAttribute {
        wgpu::VertexFormat format;
        uint64_t offset;
        uint32_t shaderLocation;
    };
    std::vector<VertexAttribute> vertexAttributes;
    uint64_t vertexStride = 0;

    // Pipeline settings
    wgpu::PrimitiveTopology topology = wgpu::PrimitiveTopology::TriangleList;
    bool hasDepthStencil = false;

    /**
     * @brief Load shader source from file
     * @param filepath Path to shader file (.wgsl)
     * @return Shader source code as string
     */
    static std::string loadShaderFromFile(const std::string& filepath);
};

// NOTE: ShaderType enum and ShaderFactory/ShaderBindingBuilder classes
// have been moved to shader/ShaderLibrary.h for better organization.
// Please use clipengine::ShaderLibrary instead.

/**
 * @brief Predefined shader configurations
 */
class ShaderPresets {
public:
    /**
     * @brief NV12 video format shader (Y plane + UV plane)
     */
    static ShaderConfig createNV12VideoShader();

    /**
     * @brief RGBA texture shader
     */
    static ShaderConfig createRGBATextureShader();

    /**
     * @brief I420 video format shader (Y plane + U plane + V plane)
     */
    static ShaderConfig createI420VideoShader();

    /**
     * @brief Simple color shader (for testing)
     */
    static ShaderConfig createColorShader();

    /**
     * @brief Panorama shader for RGBA texture (360° equirectangular)
     */
    static ShaderConfig createPanoramaRGBAShader();

    /**
     * @brief Panorama shader for NV12 format (360° equirectangular)
     */
    static ShaderConfig createPanoramaNV12Shader();
};
