#pragma once

#include "TextureRenderer.h"
#include "ShaderConfig.h"

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
 * @brief Video renderer that supports various video formats
 *
 * This renderer extends TextureRenderer to handle D3D11 texture import
 * and multi-plane video formats (NV12, I420).
 */
class VideoRenderer : public TextureRenderer {
public:
    /**
     * @brief Construct a new Video Renderer with default NV12 format
     */
    VideoRenderer();

    /**
     * @brief Construct a new Video Renderer with custom shader config
     * @param config Custom shader configuration
     */
    explicit VideoRenderer(const ShaderConfig& config);

    /**
     * @brief Construct a new Video Renderer with specific video format
     * @param format Video format (NV12, I420, RGBA)
     */
    explicit VideoRenderer(VideoFormat format);

    ~VideoRenderer() override;

    /**
     * @brief Update the video frame from D3D11 texture
     * @param texture D3D11 texture containing video frame
     * @param arrayIndex Array index for texture arrays
     * @return true if successful, false otherwise
     */
    bool updateFrame(ID3D11Texture2D* texture, int arrayIndex = 0);

    /**
     * @brief Set the video format (will recreate shader pipeline)
     * @param format Video format to use
     */
    void setVideoFormat(VideoFormat format);

    VideoFormat getVideoFormat() const { return videoFormat_; }

    enum class FillMode {
        Fit,
        Fill,
        Stretch
    };
    void setFillMode(FillMode mode) { fillMode_ = mode; }

private:
    void createShaderForFormat();

    VideoFormat videoFormat_ = VideoFormat::NV12;
    FillMode fillMode_ = FillMode::Fit;

    // Cached texture views
    wgpu::TextureView yPlaneView_;
    wgpu::TextureView uvPlaneView_;
    wgpu::TextureView uPlaneView_;
    wgpu::TextureView vPlaneView_;
};
