#pragma once

#include "Layer.h"

namespace clipengine {

/**
 * @brief Image layer for rendering static images
 *
 * ImageLayer loads and renders static image content:
 * - PNG, JPEG, BMP, TGA, etc. (via stb_image)
 * - GPU textures
 * - Memory buffers
 *
 * Example usage:
 * @code
 * auto imageLayer = std::make_shared<ImageLayer>();
 * imageLayer->loadImage("photo.png");
 * imageLayer->transform.position = {100.0f, 200.0f};
 * imageLayer->transform.scale = {0.5f, 0.5f};
 * @endcode
 */
class ImageLayer : public Layer {
public:
    ImageLayer();
    ~ImageLayer() override;

    // ========================================================================
    // Layer Interface Implementation
    // ========================================================================

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    wgpu::TextureView render(float time) override;
    void update(float deltaTime) override;
    LayerType getType() const override { return LayerType::Image; }
    glm::vec2 getSize() const override;

    // ========================================================================
    // Image Loading
    // ========================================================================

    /**
     * @brief Load image from file path
     * @param path Path to image file (PNG, JPEG, BMP, etc.)
     * @return true if successful
     */
    bool loadImage(const std::string& path);

    /**
     * @brief Load image from memory buffer
     * @param data RGBA pixel data
     * @param width Image width
     * @param height Image height
     * @return true if successful
     */
    bool loadImage(const void* data, uint32_t width, uint32_t height);

    /**
     * @brief Load image from existing texture
     * @param texture WebGPU texture
     * @param width Image width
     * @param height Image height
     * @return true if successful
     */
    bool loadTexture(wgpu::Texture texture, uint32_t width, uint32_t height);

    // ========================================================================
    // Image Properties
    // ========================================================================

    /**
     * @brief Get current image resolution
     */
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }

private:
    void createPipeline();
    void createBindGroup();

    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // GPU resources
    wgpu::Texture texture_;
    wgpu::TextureView textureView_;
    wgpu::Sampler sampler_;
    wgpu::Buffer vertexBuffer_;
    wgpu::Buffer uniformBuffer_;
    wgpu::BindGroup bindGroup_;
    wgpu::RenderPipeline pipeline_;
    wgpu::Texture renderTarget_;
    wgpu::TextureView renderTargetView_;
};

} // namespace clipengine
