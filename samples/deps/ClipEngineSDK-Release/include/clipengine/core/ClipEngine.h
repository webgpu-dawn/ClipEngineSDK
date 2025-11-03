/**
 * @file ClipEngine.h
 * @brief Core engine class for ClipEngine SDK
 * @author ClipEngine Team
 * @version 1.0.0
 *
 * This file contains the main engine interface for the ClipEngine SDK.
 * ClipEngine manages the rendering context, renderers, and frame updates.
 */

#pragma once

#include <memory>
#include <vector>
#include <cstdint>

// Forward declarations
class CeRenderable;
class CeContext;
struct CeConfigure;

enum class CeRendererType : int;

// Note: WebGPU types are intentionally forward declared to minimize dependencies
// The actual types will be available when linking against the ClipEngine library
namespace wgpu {
    class Device;
    class Queue;
    enum class TextureFormat : uint32_t;
}



class ClipEngine {
public:
    /**
     * @brief Default constructor
     */
    ClipEngine();

    /**
     * @brief Destructor - cleans up engine resources
     */
    ~ClipEngine();

    bool initialize(const CeConfigure& config);

    void shutdown();

    /**
     * @brief Add a renderer to the engine
     *
     * The renderer will be added to the internal renderer list and
     * automatically sorted by layer order. The engine takes ownership
     * of the renderer.
     *
     * @param renderer Unique pointer to the renderer to add
     */
    void addRenderer(std::unique_ptr<CeRenderable> renderer);

    /**
     * @brief Remove a renderer from the engine
     *
     * @param renderer Pointer to the renderer to remove
     */
    void removeRenderer(CeRenderable* renderer);

    /**
     * @brief Get a renderer by its name
     *
     * @param name The name of the renderer to search for
     * @return Pointer to the renderer if found, nullptr otherwise
     */
    CeRenderable* getRendererByName(const char* name);

    /**
     * @brief Clear all renderers from the engine
     */
    void clear();

    // ========================================================================
    // Rendering Control
    // ========================================================================

    /**
     * @brief Render a single frame
     *
     * This method renders all registered renderers in layer order,
     * presents the frame, and handles GPU timing if enabled.
     */
    void renderFrame();

    /**
     * @brief Update engine state and all renderers
     *
     * @param deltaTime Time elapsed since last update in seconds
     */
    void update(float deltaTime);

    // ========================================================================
    // GPU Resource Access
    // ========================================================================

    /**
     * @brief Get the WebGPU device
     *
     * @return The WebGPU device handle
     */
    wgpu::Device getDevice() const;

    /**
     * @brief Get the WebGPU command queue
     *
     * @return The WebGPU queue handle
     */
    wgpu::Queue getQueue() const;

    /**
     * @brief Get the surface texture format
     *
     * @return The texture format used by the rendering surface
     */
    wgpu::TextureFormat getSurfaceFormat() const;

    /**
     * @brief Get the rendering context
     *
     * Provides access to the underlying rendering context for advanced use cases.
     *
     * @return Reference to the CeContext object
     */
    CeContext& getContext();

    /**
     * @brief Get the rendering context (const version)
     *
     * @return Const reference to the CeContext object
     */
    const CeContext& getContext() const;

private:
    // ========================================================================
    // Implementation Details (Pimpl Idiom)
    // ========================================================================

    /**
     * @brief Opaque pointer to implementation details
     *
     * This hides all internal implementation details from the public API,
     * reducing compilation dependencies and improving ABI stability.
     */
    class Impl;
    std::unique_ptr<Impl> pimpl_;

    // Helper method for template implementation
    std::vector<CeRenderable*> getRenderersInternal();
    const std::vector<CeRenderable*> getRenderersInternal() const;
};