#pragma once

#include "TextureRenderer.h"
#include "ShaderConfig.h"

/**
 * @brief Renderer for planar panorama (equirectangular to planar UV mapping)
 *
 * This renderer samples an equirectangular texture and maps it to a 2D plane
 * while supporting rotation (yaw, pitch) and zoom.
 */
class PanoramaRenderer : public TextureRenderer {
public:
    PanoramaRenderer();
    ~PanoramaRenderer() override;

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void render(wgpu::RenderPassEncoder& pass) override;
    void update(float deltaTime) override;

    void setRotation(float yawRadians, float pitchRadians);
    void setZoom(float zoom); // 1.0 = default
    /**
     * @brief Apply texture views (either single RGBA or NV12 Y+UV) to the panorama renderer
     */
    void applyTextureViews(const std::vector<wgpu::TextureView>& views);

private:
    void updateUniformsIfNeeded();

    struct Uniforms {
        float yaw;
        float pitch;
        float zoom;
        float aspect; // aspect ratio width/height
    } uniforms_ = {0.0f, 0.0f, 1.0f, 1.0f};

    bool uniformsDirty_ = true;
public:
    void setAspect(float aspect) { uniforms_.aspect = aspect; uniformsDirty_ = true; }
};
