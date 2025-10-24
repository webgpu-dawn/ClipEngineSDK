#pragma once

#include "Filter.h"
#include <vector>
#include <memory>

/**
 * @brief Manages a chain of filters applied sequentially
 *
 * FilterChain handles the creation of intermediate render textures
 * and applies filters in order. The final output is rendered to
 * the provided render pass.
 */
class FilterChain {
public:
    FilterChain() = default;
    ~FilterChain() = default;

    // Allow move construction and assignment
    FilterChain(FilterChain&&) = default;
    FilterChain& operator=(FilterChain&&) = default;
    // Disable copy
    FilterChain(const FilterChain&) = delete;
    FilterChain& operator=(const FilterChain&) = delete;

    /**
     * @brief Initialize the filter chain
     * @param device WebGPU device
     * @param format Texture format for intermediate textures
     * @param width Width of intermediate textures
     * @param height Height of intermediate textures
     */
    bool initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height);

    /**
     * @brief Add a filter to the end of the chain
     * @param filter Unique pointer to the filter (ownership transferred)
     */
    void addFilter(std::unique_ptr<Filter> filter);

    /**
     * @brief Remove filter at index
     * @param index Filter index to remove
     */
    void removeFilter(size_t index);

    /**
     * @brief Clear all filters
     */
    void clearFilters();

    /**
     * @brief Get filter at index
     */
    Filter* getFilter(size_t index);

    /**
     * @brief Get number of filters
     */
    size_t getFilterCount() const { return filters_.size(); }

    /**
     * @brief Apply all filters in the chain
     * @param encoder Command encoder for creating render passes
     * @param inputTextures Input texture views
     * @param outputView Final output texture view (usually the render target)
     * @param inputState Optional input state for interactive effects
     * @return true if successful
     */
    bool apply(wgpu::CommandEncoder& encoder,
               const std::vector<wgpu::TextureView>& inputTextures,
               wgpu::TextureView outputView,
               const InputState* inputState = nullptr);

    /**
     * @brief Resize intermediate textures
     */
    void resize(uint32_t width, uint32_t height);

    /**
     * @brief Check if chain has any filters
     */
    bool isEmpty() const { return filters_.empty(); }

private:
    wgpu::Device device_;
    wgpu::TextureFormat format_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    std::vector<std::unique_ptr<Filter>> filters_;

    // Intermediate textures for filter chain (ping-pong buffers)
    wgpu::Texture intermediateTexture0_;
    wgpu::Texture intermediateTexture1_;
    wgpu::TextureView intermediateView0_;
    wgpu::TextureView intermediateView1_;

    void createIntermediateTextures();
};
