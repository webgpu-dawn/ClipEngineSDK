#pragma once

#include "VideoRenderer.h"
#include "ShaderConfig.h"

/**
 * @brief Flat panorama renderer with rotation and zoom support
 */
class FlatPanoramaRenderer : public VideoRenderer {
public:
    /**
     * @brief Construct a new Flat Panorama Renderer
     */
    FlatPanoramaRenderer();
    ~FlatPanoramaRenderer() override;

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void update(float deltaTime) override;
    
    /**
     * @brief Set rotation angle in radians
     */
    void setRotation(float radians);
    
    /**
     * @brief Set zoom scale factor (1.0 = normal size)
     */
    void setScale(float scale);
    
    /**
     * @brief Set position offset
     */
    void setOffset(float x, float y);
    
    /**
     * @brief Get current rotation in radians
     */
    float getRotation() const { return rotation_; }
    
    /**
     * @brief Get current scale factor
     */
    float getScale() const { return scale_; }
    
    /**
     * @brief Get current X offset
     */
    float getOffsetX() const { return offsetX_; }
    
    /**
     * @brief Get current Y offset
     */
    float getOffsetY() const { return offsetY_; }

private:
    void updateTransformBuffer();
    
    float rotation_ = 0.0f;
    float scale_ = 1.0f;
    float offsetX_ = 0.0f;
    float offsetY_ = 0.0f;
    
    wgpu::Buffer transformBuffer_;
    
    struct TransformUniforms {
        float rotation;
        float scale;
        float offsetX;
        float offsetY;
    };
};