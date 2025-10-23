#pragma once

#include "../core/CeRenderable.h"
#include "ShaderConfig.h"

#include <memory>
#include <vector>

/**
 * @brief Generic texture renderer that can use any shader configuration
 *
 * This renderer accepts a ShaderConfig and creates a rendering pipeline
 * based on that configuration. It's flexible enough to handle various
 * texture formats and shader types.
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
     * @brief Get the shader configuration used by this renderer
     */
    const ShaderConfig& getShaderConfig() const { return shaderConfig_; }

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
};
