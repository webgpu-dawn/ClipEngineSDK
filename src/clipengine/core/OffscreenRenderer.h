#pragma once

#include "../utils/Common.h"
#include <vector>
#include <functional>

/**
 * @brief Offscreen renderer for video export
 *
 * OffscreenRenderer provides a rendering target that can be read back to CPU memory,
 * enabling video export functionality. Instead of rendering to a window surface,
 * frames are rendered to a texture that can be captured and encoded to video.
 *
 * Features:
 * - Render to texture (no window required)
 * - Async pixel readback via staging buffer
 * - Support for various output formats (RGBA8, BGRA8)
 * - Frame callback system for video encoders
 *
 * Example usage:
 * @code
 * OffscreenRenderer renderer;
 * renderer.initialize(device, wgpu::TextureFormat::RGBA8Unorm, 1920, 1080);
 *
 * // Render frame
 * engine.update(1.0f / 60.0f);
 * engine.render(renderer.getTargetView());
 *
 * // Read pixels for video encoding
 * renderer.readPixels([](const uint8_t* data, size_t size) {
 *     // Encode frame to video...
 * });
 * @endcode
 */
class OffscreenRenderer {
public:
    OffscreenRenderer() = default;
    ~OffscreenRenderer();

    /**
     * @brief Initialize the offscreen renderer
     * @param device WebGPU device
     * @param format Target texture format (RGBA8Unorm or BGRA8Unorm recommended)
     * @param width Width in pixels
     * @param height Height in pixels
     * @return true if initialization succeeded
     */
    bool initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height);

    /**
     * @brief Get the rendering target view
     * Pass this to CompositionEngine::render(view)
     * @return Texture view for rendering
     */
    wgpu::TextureView getTargetView() const { return renderTargetView_; }

    /**
     * @brief Get the offscreen texture
     * @return Offscreen texture
     */
    wgpu::Texture getTexture() const { return renderTargetTexture_; }

    /**
     * @brief Read rendered pixels to CPU memory (async)
     *
     * This initiates an async GPU→CPU transfer. The callback will be invoked
     * once the data is ready. Call waitForCompletion() to block until ready.
     *
     * @param callback Function called when pixels are ready
     *                 Parameters: (data pointer, data size in bytes)
     */
    void readPixels(std::function<void(const uint8_t*, size_t)> callback);

    /**
     * @brief Read pixels synchronously (blocking)
     *
     * Blocks until pixel data is available and copies to the provided buffer.
     * Buffer must be large enough (width * height * 4 bytes for RGBA8).
     *
     * @param buffer Destination buffer
     * @param bufferSize Size of destination buffer in bytes
     * @return true if read succeeded, false if buffer too small or error
     */
    bool readPixelsSync(uint8_t* buffer, size_t bufferSize);

    /**
     * @brief Wait for pending GPU operations to complete
     * Call this after readPixels() to ensure data is ready
     */
    void waitForCompletion();

    /**
     * @brief Get texture width
     */
    uint32_t getWidth() const { return width_; }

    /**
     * @brief Get texture height
     */
    uint32_t getHeight() const { return height_; }

    /**
     * @brief Get texture format
     */
    wgpu::TextureFormat getFormat() const { return format_; }

    /**
     * @brief Get bytes per pixel for current format
     */
    uint32_t getBytesPerPixel() const;

    /**
     * @brief Get total buffer size needed for pixel data
     */
    size_t getBufferSize() const {
        return width_ * height_ * getBytesPerPixel();
    }

private:
    wgpu::Device device_;
    wgpu::Texture renderTargetTexture_;
    wgpu::TextureView renderTargetView_;
    wgpu::Buffer stagingBuffer_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    wgpu::TextureFormat format_ = wgpu::TextureFormat::RGBA8Unorm;

    std::function<void(const uint8_t*, size_t)> pendingCallback_;

    void createRenderTarget();
    void createStagingBuffer();
};
