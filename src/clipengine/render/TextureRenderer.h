#pragma once

#include "../core/CeRenderable.h"
#include "ShaderConfig.h"
#include "FilterChain.h"

#include <memory>
#include <vector>

/**
 * @brief Generic texture renderer that can use any shader configuration
 *
 * This renderer accepts a ShaderConfig and creates a rendering pipeline
 * based on that configuration. It's flexible enough to handle various
 * texture formats and shader types.
 *
 * Now supports filter chains for applying post-processing effects.
 */
class TextureRenderer : public CeRenderable {
public:
    /**
     * @brief Construct a new Texture Renderer object
     * @param config Shader configuration to use
     */
    explicit TextureRenderer(const ShaderConfig& config);
    ~TextureRenderer() override;

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void render(wgpu::RenderPassEncoder& pass) override;
    void update(float deltaTime) override;
    CeRendererType getType() const override { return CeRendererType::Video; }
    void setViewport(float x, float y, float width, float height) override;

    /**
     * @brief Update texture bindings for rendering
     * @param textureViews List of texture views to bind (must match shader config)
     */
    void updateTextures(const std::vector<wgpu::TextureView>& textureViews);

    /**
     * @brief Get current texture views used by this renderer
     */
    const std::vector<wgpu::TextureView>& getTextureViews() const { return textureViews_; }

    /**
     * @brief Get the shader configuration used by this renderer
     */
    const ShaderConfig& getShaderConfig() const { return shaderConfig_; }

    /**
     * @brief Get the filter chain for adding/managing filters
     */
    FilterChain& getFilterChain() { return filterChain_; }
    const FilterChain& getFilterChain() const { return filterChain_; }

protected:
    void initializeBuffers();
    void initializeSampler();
    void initializeShader();
    void initializePipeline();
    void updateBindGroup();
    void updateVertexBuffer();

    ShaderConfig shaderConfig_;

    wgpu::Buffer vertexBuffer_;
    wgpu::Sampler sampler_;
    wgpu::ShaderModule vertexShaderModule_;
    wgpu::ShaderModule fragmentShaderModule_;
    wgpu::RenderPipeline pipeline_;
    wgpu::BindGroup bindGroup_;
    wgpu::BindGroupLayout bindGroupLayout_;

    std::vector<wgpu::TextureView> textureViews_;
    // Optional uniform buffer (used by shaders that declare a uniform / storage buffer)
    wgpu::Buffer uniformBuffer_;
    uint64_t uniformBufferSize_ = 0;

    /**
     * @brief Update uniform buffer contents
     * @param data Pointer to data to upload
     * @param size Size in bytes (must be <= uniform buffer size)
     */
    void updateUniformData(const void* data, size_t size);

    // Filter chain support
    FilterChain filterChain_;
};
