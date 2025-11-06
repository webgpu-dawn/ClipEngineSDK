#pragma once

#include "Layer.h"
#include "../shaders/ShaderConfig.h"

#if _WIN32
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#endif

namespace clipengine {

/**
 * @brief Video format enumeration
 */
enum class VideoFormat {
    NV12,    ///< NV12 format (Y plane + UV interleaved)
    I420,    ///< I420 format (Y, U, V separate planes)
    RGBA     ///< RGBA format (single plane)
};

/**
 * @brief Video projection mode
 */
enum class VideoProjection {
    Planar,         ///< Normal flat video (default)
    Equirectangular,///< 360° panoramic video (equirectangular projection)
    LittlePlanet,   ///< Little planet effect (stereographic projection)
    CrystalBall     ///< Crystal ball effect (fisheye/inverse stereographic)
};

/**
 * @brief Video layer for rendering video content
 *
 * VideoLayer loads and renders video frames from various sources:
 * - Video files (via FFmpeg decoder)
 * - D3D11 textures (hardware decode)
 * - Memory buffers
 *
 * Supports:
 * - Multiple video formats (NV12, I420, RGBA)
 * - Multiple projection modes (Planar, 360° panoramic, etc.)
 * - Interactive controls (rotation, zoom for panoramic videos)
 *
 * Example usage:
 * @code
 * auto videoLayer = std::make_shared<VideoLayer>();
 * videoLayer->loadVideo("video.mp4");
 * videoLayer->setProjection(VideoProjection::Equirectangular);
 * videoLayer->setRotation(0.5f, 0.3f);  // yaw, pitch
 * @endcode
 */
class VideoLayer : public Layer {
public:
    VideoLayer();
    explicit VideoLayer(VideoFormat format);
    ~VideoLayer() override;

    // ========================================================================
    // Layer Interface Implementation
    // ========================================================================

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    wgpu::TextureView render(float time) override;
    void update(float deltaTime) override;
    LayerType getType() const override { return LayerType::Video; }
    glm::vec2 getSize() const override;

    // ========================================================================
    // Video Loading
    // ========================================================================

    /**
     * @brief Load video from file path
     * @param path Path to video file
     * @return true if successful
     */
    bool loadVideo(const std::string& path);

    /**
     * @brief Update video frame from D3D11 texture
     * @param texture D3D11 texture containing video frame
     * @param arrayIndex Array index for texture arrays
     * @return true if successful
     */
    bool updateFrame(ID3D11Texture2D* texture, int arrayIndex = 0);

    /**
     * @brief Update video frame from memory buffer
     * @param data Pixel data
     * @param width Frame width
     * @param height Frame height
     * @param format Pixel format
     * @return true if successful
     */
    bool updateFrame(const void* data, uint32_t width, uint32_t height, VideoFormat format);

    // ========================================================================
    // Video Properties
    // ========================================================================

    /**
     * @brief Set video format (will recreate pipeline)
     */
    void setVideoFormat(VideoFormat format);
    VideoFormat getVideoFormat() const { return videoFormat_; }

    /**
     * @brief Set projection mode (planar, panoramic, etc.)
     */
    void setProjection(VideoProjection projection);
    VideoProjection getProjection() const { return projection_; }

    /**
     * @brief Set rotation for panoramic videos (in radians)
     * @param yaw Horizontal rotation
     * @param pitch Vertical rotation
     */
    void setRotation(float yaw, float pitch);

    /**
     * @brief Set zoom level for panoramic videos
     * @param zoom Zoom factor (1.0 = default, >1.0 = zoom in)
     */
    void setZoom(float zoom);

    /**
     * @brief Set aspect ratio
     */
    void setAspect(float aspect);

    /**
     * @brief Get current video resolution
     */
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }

private:
    void createShaderForProjection();
    void updatePanoramaUniforms();
    void cleanupTextures();

    // Video properties
    VideoFormat videoFormat_ = VideoFormat::NV12;
    VideoProjection projection_ = VideoProjection::Planar;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // Panorama parameters
    struct PanoramaParams {
        float yaw = 0.0f;
        float pitch = 0.0f;
        float zoom = 1.0f;
        float aspect = 16.0f / 9.0f;
    } panoramaParams_;

    bool panoramaUniformsDirty_ = true;
    bool pipelineNeedsRebuild_ = false;

    // GPU resources
    wgpu::Texture texture_;
    wgpu::TextureView textureView_;
    wgpu::Sampler sampler_;
    wgpu::Buffer uniformBuffer_;
    wgpu::BindGroup bindGroup_;
    wgpu::RenderPipeline pipeline_;

    // D3D11 interop (Windows only)
#if _WIN32
    struct SharedTextureData {
        ComPtr<ID3D11Texture2D> texture;
        HANDLE handle = nullptr;
    } sharedTexture_;

    struct DawnTextureData {
        wgpu::Texture texture;
        wgpu::SharedTextureMemory sharedMemory;
    } dawnTexture_;
#endif
};

} // namespace clipengine
