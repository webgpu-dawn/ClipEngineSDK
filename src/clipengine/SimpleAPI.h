#pragma once

#include "compositor/Compositor.h"
#include "layers/Layer.h"
#include "layers/VideoLayer.h"
#include "layers/ImageLayer.h"
#include "layers/SolidLayer.h"
#include "core/RenderDevice.h"
#include "utils/Common.h"
#include "utils/Vec4.h"
#include <memory>
#include <string>

namespace clipengine {

/**
 * @brief Simplified ClipEngine API for easy video composition
 *
 * SimpleClipEngine provides a high-level, user-friendly API based on the
 * Layer + Transform + Filter Chain + Compositor architecture.
 *
 * Example usage:
 * @code
 * // Initialize engine
 * SimpleClipEngine engine;
 * engine.initialize(1920, 1080);
 *
 * // Create layers
 * auto background = engine.createSolidLayer({0.2f, 0.2f, 0.2f, 1.0f});
 * auto video = engine.createVideoLayer("input.mp4");
 * auto logo = engine.createImageLayer("logo.png");
 *
 * // Transform layers
 * video->transform.position = {960, 540};  // Center
 * logo->transform.position = {100, 100};   // Top-left
 * logo->transform.scale = {0.5f, 0.5f};    // 50% size
 *
 * // Render
 * engine.render();
 * @endcode
 */
class SimpleClipEngine {
public:
    SimpleClipEngine();
    ~SimpleClipEngine();

    // ========================================================================
    // Initialization
    // ========================================================================

    /**
     * @brief Initialize the engine
     * @param width Output width in pixels
     * @param height Output height in pixels
     * @param windowHandle Native window handle (optional, for display)
     * @return true if successful
     */
    bool initialize(uint32_t width, uint32_t height, void* windowHandle = nullptr);

    /**
     * @brief Shutdown the engine
     */
    void shutdown();

    bool isInitialized() const { return initialized_; }

    // ========================================================================
    // Layer Creation
    // ========================================================================

    /**
     * @brief Create a video layer from file
     */
    std::shared_ptr<VideoLayer> createVideoLayer(
        const std::string& videoPath,
        const std::string& name = ""
    );

    /**
     * @brief Create an image layer from file
     */
    std::shared_ptr<ImageLayer> createImageLayer(
        const std::string& imagePath,
        const std::string& name = ""
    );

    /**
     * @brief Create a solid color layer
     */
    std::shared_ptr<SolidLayer> createSolidLayer(
        const Vec4& color,
        const std::string& name = ""
    );

    /**
     * @brief Create an empty video layer
     */
    std::shared_ptr<VideoLayer> createVideoLayer(const std::string& name = "");

    /**
     * @brief Create an empty image layer
     */
    std::shared_ptr<ImageLayer> createImageLayer(const std::string& name = "");

    // ========================================================================
    // Layer Management
    // ========================================================================

    void addLayer(std::shared_ptr<Layer> layer, int zOrder = -1);
    void removeLayer(const std::shared_ptr<Layer>& layer);
    void removeLayerByName(const std::string& name);
    std::shared_ptr<Layer> getLayerByName(const std::string& name);
    const std::vector<std::shared_ptr<Layer>>& getLayers() const;
    void clearLayers();

    // ========================================================================
    // Rendering
    // ========================================================================

    wgpu::TextureView render(float time = 0.0f);
    void update(float deltaTime);
    void present();

    // ========================================================================
    // Configuration
    // ========================================================================

    void setResolution(uint32_t width, uint32_t height);
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }
    void setBackgroundColor(const Vec4& color);

    // ========================================================================
    // Advanced Access
    // ========================================================================

    wgpu::Device getDevice() const { return device_; }
    Compositor& getCompositor() { return compositor_; }
    LayerStack& getLayerStack() { return layerStack_; }

private:
    bool initialized_ = false;
    uint32_t width_ = 1920;
    uint32_t height_ = 1080;

    wgpu::Device device_;
    wgpu::TextureFormat format_ = wgpu::TextureFormat::BGRA8Unorm;

    Compositor compositor_;
    LayerStack layerStack_;

    float currentTime_ = 0.0f;
    int nextZOrder_ = 0;
};

} // namespace clipengine
