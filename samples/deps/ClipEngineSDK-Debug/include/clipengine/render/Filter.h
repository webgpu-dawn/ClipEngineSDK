#pragma once

#include "../common/Common.h"
#include "ShaderConfig.h"
#include "InputState.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Base class for all texture filters
 *
 * Filters process input textures and output to render targets.
 * Each filter can have adjustable parameters stored in uniform buffers.
 */
class Filter {
public:
    virtual ~Filter() = default;

    /**
     * @brief Initialize the filter with a WebGPU device
     * @param device WebGPU device
     * @param format Output texture format
     * @return true if initialization succeeded
     */
    virtual bool initialize(wgpu::Device device, wgpu::TextureFormat format) = 0;

    /**
     * @brief Apply the filter to input texture(s) and render to the given pass
     * @param pass Render pass encoder to record commands
     * @param inputTextures Input texture views (usually 1, but can be multiple)
     */
    virtual void apply(wgpu::RenderPassEncoder& pass, const std::vector<wgpu::TextureView>& inputTextures) = 0;

    /**
     * @brief Update filter parameters (called before apply)
     */
    virtual void updateParameters() = 0;

    /**
     * @brief Get filter name for debugging
     */
    virtual const std::string& getName() const = 0;

    /**
     * @brief Check if filter is enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Enable or disable the filter
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Set input state reference (called by engine)
     */
    void setInputState(const InputState* input) { inputState_ = input; }

protected:
    wgpu::Device device_;
    wgpu::TextureFormat outputFormat_;
    bool enabled_ = true;
    const InputState* inputState_ = nullptr;

    // Common resources
    wgpu::Buffer vertexBuffer_;
    wgpu::Sampler sampler_;
    wgpu::ShaderModule vertexShaderModule_;
    wgpu::ShaderModule fragmentShaderModule_;
    wgpu::RenderPipeline pipeline_;
    wgpu::BindGroup bindGroup_;
    wgpu::BindGroupLayout bindGroupLayout_;
    wgpu::Buffer uniformBuffer_;
    uint64_t uniformBufferSize_ = 0;

    /**
     * @brief Create full-screen quad vertex buffer
     */
    void createFullScreenQuad();

    /**
     * @brief Create a default sampler
     */
    void createSampler();

    /**
     * @brief Update uniform buffer with new data
     */
    void updateUniformBuffer(const void* data, size_t size);
};

/**
 * @brief Base class for filters using ShaderConfig
 */
class ShaderFilter : public Filter {
public:
    explicit ShaderFilter(const std::string& name, const ShaderConfig& config);

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void apply(wgpu::RenderPassEncoder& pass, const std::vector<wgpu::TextureView>& inputTextures) override;
    void updateParameters() override {}
    const std::string& getName() const override { return name_; }

protected:
    std::string name_;
    ShaderConfig shaderConfig_;
    std::vector<wgpu::TextureView> currentInputs_;

    void initializeShader();
    void initializePipeline();
    void updateBindGroup();
};
