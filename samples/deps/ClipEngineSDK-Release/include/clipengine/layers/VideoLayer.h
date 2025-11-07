#pragma once

#include "ImageLayer.h"
#include "../shaders/ShaderConfig.h"

#include <memory>

#if _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#elif __APPLE__
#endif

enum class VideoFormat {
    NV12,
    I420,
    RGBA
};

/**
 * @brief Unified video layer supporting both planar and panoramic modes
 *
 * This layer extends ImageLayer to handle:
 * - D3D11 texture import
 * - Multi-plane video formats (NV12, I420, RGBA)
 * - Planar rendering (normal video)
 * - Panoramic rendering (360° equirectangular video)
 */
class VideoLayer : public ImageLayer {
public:
    // Internal structures for shared texture management
    struct SharedTextureData {
        ComPtr<ID3D11Texture2D> texture;
        HANDLE handle = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    struct DawnTextureData {
        wgpu::Texture texture;
        wgpu::SharedTextureMemory sharedMemory;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    /**
     * @brief Render mode for video display
     */
    enum class RenderMode {
        Planar,        // Normal flat video
        Panorama,      // 360° panoramic video (equirectangular projection)
        LittlePlanet,  // Little planet effect (stereographic projection)
        CrystalBall    // Crystal ball effect (fisheye/inverse stereographic)
    };

    enum class FillMode {
        Fit,
        Fill,
        Stretch
    };
    /**
     * @brief Construct a new Video Layer with default NV12 format
     */
    VideoLayer();

    /**
     * @brief Construct a new Video Layer with custom shader config
     * @param config Custom shader configuration
     */
    explicit VideoLayer(const ShaderConfig& config);

    /**
     * @brief Construct a new Video Layer with specific video format
     * @param format Video format (NV12, I420, RGBA)
     */
    explicit VideoLayer(VideoFormat format);

    ~VideoLayer() override;

    /**
     * @brief Update the video frame from D3D11 texture
     * @param texture D3D11 texture containing video frame
     * @param arrayIndex Array index for texture arrays
     * @return true if successful, false otherwise
     */
    bool updateFrame(ID3D11Texture2D* texture, int arrayIndex = 0);

    /**
     * @brief Load an image from file and update the frame
     *
     * Supports multiple image formats: JPEG, PNG, BMP, TGA, PSD, GIF, HDR, PIC, PNM
     * Images are automatically converted to RGBA format for GPU upload.
     *
     * @param filename Path to the image file
     * @return true if successful, false otherwise
     */
    bool loadFromFile(const std::string& filename);

    /**
     * @brief Set the video format (will recreate shader pipeline)
     * @param format Video format to use
     */
    void setVideoFormat(VideoFormat format);

    VideoFormat getVideoFormat() const { return videoFormat_; }

    /**
     * @brief Set render mode (planar or panorama)
     * This will switch the shader pipeline accordingly
     * @param mode Render mode to use
     */
    void setRenderMode(RenderMode mode);
    RenderMode getRenderMode() const { return renderMode_; }

    /**
     * @brief Set rotation for panorama mode (radians)
     * @param yawRadians Horizontal rotation
     * @param pitchRadians Vertical rotation
     */
    void setRotation(float yawRadians, float pitchRadians);

    /**
     * @brief Set zoom level for panorama mode
     * @param zoom Zoom factor (1.0 = default, >1.0 = zoom in, <1.0 = zoom out)
     */
    void setZoom(float zoom);

    /**
     * @brief Set aspect ratio for panorama mode
     * @param aspect Aspect ratio (width/height)
     */
    void setAspect(float aspect);

    void setFillMode(FillMode mode) { fillMode_ = mode; }

    void update(float deltaTime) override;

private:
    void createShaderForMode();
    void updatePanoramaUniforms();

    VideoFormat videoFormat_ = VideoFormat::NV12;
    FillMode fillMode_ = FillMode::Fit;
    RenderMode renderMode_ = RenderMode::Planar;

    // Panorama parameters
    struct PanoramaParams {
        float yaw = 0.0f;
        float pitch = 0.0f;
        float zoom = 1.0f;
        float aspect = 16.0f / 9.0f;
    } panoramaParams_;
    bool panoramaUniformsDirty_ = true;

    // Pipeline rebuild state
    bool pipelineNeedsRebuild_ = false;
    RenderMode pendingRenderMode_ = RenderMode::Planar;

    // Cached texture views
    wgpu::TextureView yPlaneView_;
    wgpu::TextureView uvPlaneView_;
    wgpu::TextureView uPlaneView_;
    wgpu::TextureView vPlaneView_;

    // Shared texture data (instance-specific, not static)
    SharedTextureData sharedTextureData_;
    DawnTextureData dawnTextureData_;

    void cleanupSharedTextures();
};
