#pragma once

#include "../layers/Layer.h"
#include "../utils/Common.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace clipengine {

/**
 * @brief Layer stack for managing multiple layers
 *
 * LayerStack maintains a collection of layers and manages their z-order.
 * Layers with lower z-order render first (background), higher z-order on top (foreground).
 */
class LayerStack {
public:
    /**
     * @brief Add a layer to the stack
     * @param layer Layer to add
     */
    void addLayer(std::shared_ptr<Layer> layer) {
        layers_.push_back(layer);
        sortByZOrder();
    }

    /**
     * @brief Remove a layer by index
     */
    void removeLayer(size_t index) {
        if (index < layers_.size()) {
            layers_.erase(layers_.begin() + index);
        }
    }

    /**
     * @brief Remove a layer by pointer
     */
    void removeLayer(const std::shared_ptr<Layer>& layer) {
        auto it = std::find(layers_.begin(), layers_.end(), layer);
        if (it != layers_.end()) {
            layers_.erase(it);
        }
    }

    /**
     * @brief Remove a layer by name
     */
    void removeLayerByName(const std::string& name) {
        auto it = std::find_if(layers_.begin(), layers_.end(),
            [&name](const std::shared_ptr<Layer>& layer) {
                return layer->getName() == name;
            });
        if (it != layers_.end()) {
            layers_.erase(it);
        }
    }

    /**
     * @brief Clear all layers
     */
    void clear() {
        layers_.clear();
    }

    /**
     * @brief Get layer count
     */
    size_t getLayerCount() const {
        return layers_.size();
    }

    /**
     * @brief Get layer by index
     */
    std::shared_ptr<Layer> getLayer(size_t index) const {
        if (index < layers_.size()) {
            return layers_[index];
        }
        return nullptr;
    }

    /**
     * @brief Get layer by name
     */
    std::shared_ptr<Layer> getLayerByName(const std::string& name) const {
        auto it = std::find_if(layers_.begin(), layers_.end(),
            [&name](const std::shared_ptr<Layer>& layer) {
                return layer->getName() == name;
            });
        if (it != layers_.end()) {
            return *it;
        }
        return nullptr;
    }

    /**
     * @brief Get all layers (sorted by z-order)
     */
    const std::vector<std::shared_ptr<Layer>>& getLayers() const {
        return layers_;
    }

    /**
     * @brief Sort layers by z-order
     */
    void sortByZOrder() {
        std::sort(layers_.begin(), layers_.end(),
            [](const std::shared_ptr<Layer>& a, const std::shared_ptr<Layer>& b) {
                return a->getZOrder() < b->getZOrder();
            });
    }

private:
    std::vector<std::shared_ptr<Layer>> layers_;
};

/**
 * @brief Compositor for compositing multiple layers
 *
 * Compositor takes a LayerStack and renders all layers to a final output texture.
 *
 * Rendering Pipeline:
 * 1. Update all layers
 * 2. Render each layer to its own texture (applying transform + filters)
 * 3. Composite layers in z-order (applying blend modes)
 * 4. Output final composited result
 *
 * Example usage:
 * @code
 * // Create layers
 * auto background = std::make_shared<SolidLayer>(glm::vec4(0, 0, 0, 1));
 * auto video = std::make_shared<VideoLayer>();
 * auto overlay = std::make_shared<ImageLayer>();
 *
 * // Setup layer stack
 * LayerStack stack;
 * background->setZOrder(0);
 * video->setZOrder(1);
 * overlay->setZOrder(2);
 * stack.addLayer(background);
 * stack.addLayer(video);
 * stack.addLayer(overlay);
 *
 * // Render composition
 * Compositor compositor;
 * compositor.initialize(device, format, 1920, 1080);
 * wgpu::TextureView result = compositor.render(stack, currentTime);
 * @endcode
 */
class Compositor {
public:
    Compositor();
    ~Compositor();

    /**
     * @brief Initialize compositor
     * @param device WebGPU device
     * @param format Output texture format
     * @param width Output width in pixels
     * @param height Output height in pixels
     * @return true if successful
     */
    bool initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height);

    /**
     * @brief Render all layers in the stack to final output
     * @param stack Layer stack to render
     * @param time Current time in seconds
     * @return TextureView containing the composited result
     */
    wgpu::TextureView render(const LayerStack& stack, float time);

    /**
     * @brief Render all layers in the stack to the given render pass
     * @param pass Render pass encoder
     * @param stack Layer stack to render
     * @param time Current time in seconds
     */
    void renderToPass(wgpu::RenderPassEncoder& pass, const LayerStack& stack, float time);

    /**
     * @brief Update compositor (called every frame before render)
     * @param deltaTime Time since last frame
     */
    void update(float deltaTime);

    /**
     * @brief Set output resolution
     * Will recreate render targets
     */
    void setResolution(uint32_t width, uint32_t height);

    /**
     * @brief Get output resolution
     */
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }

    /**
     * @brief Set background color
     */
    void setBackgroundColor(const glm::vec4& color) {
        backgroundColor_ = color;
    }

    const glm::vec4& getBackgroundColor() const {
        return backgroundColor_;
    }

private:
    void createRenderTargets();
    void compositeLayer(wgpu::RenderPassEncoder& pass, const std::shared_ptr<Layer>& layer, float time);
    void applyLayerTransform(wgpu::RenderPassEncoder& pass, const std::shared_ptr<Layer>& layer);
    void applyBlendMode(wgpu::RenderPassEncoder& pass, BlendMode mode);

    wgpu::Device device_;
    wgpu::TextureFormat format_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // Render targets for compositing
    wgpu::Texture compositeTexture_;
    wgpu::TextureView compositeTextureView_;

    // Background color
    glm::vec4 backgroundColor_ = {0.0f, 0.0f, 0.0f, 1.0f};

    // GPU resources for compositing
    wgpu::Buffer vertexBuffer_;
    wgpu::Sampler sampler_;
    wgpu::RenderPipeline compositePipeline_;

    float lastUpdateTime_ = 0.0f;
};

} // namespace clipengine
