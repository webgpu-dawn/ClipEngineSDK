/**
 * @file CeEngine.h
 * @brief Core engine class for ClipEngine SDK
 * @author ClipEngine Team
 * @version 1.0.0
 *
 * This file contains the main engine interface for the ClipEngine SDK.
 * CeEngine manages the rendering context, renderers, and frame updates.
 */

#pragma once

// Standard library includes
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

// ClipEngine includes
#include "../common/Common.h"
#include "CeContext.h"
#include "CeRenderable.h"

// Forward declarations
struct GLFWwindow;
class GPUTimer;

/**
 * @brief Configuration structure for initializing the ClipEngine
 *
 * This structure holds all the parameters required to initialize
 * the rendering engine, including window dimensions and title.
 */
struct CeEngineConfig {
    uint32_t width = 800;               ///< Window width in pixels
    uint32_t height = 600;              ///< Window height in pixels
    const char* title = "ClipEngine";   ///< Window title
};

class CeEngine {
public:

    /**
     * @brief Default constructor
     */
    CeEngine() = default;

    /**
     * @brief Destructor - cleans up engine resources
     */
    ~CeEngine();

    /**
     * @brief Initialize the engine with the given configuration
     *
     * This method sets up the WebGPU context, creates the window,
     * and prepares the rendering pipeline.
     *
     * @param config Configuration parameters for the engine
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize(const CeEngineConfig& config);

    /**
     * @brief Shutdown the engine and release all resources
     *
     * Call this method before destroying the engine to ensure
     * proper cleanup of GPU resources and the rendering context.
     */
    void shutdown();

    // ========================================================================
    // Renderer Management
    // ========================================================================

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
    // Window Management
    // ========================================================================

    /**
     * @brief Check if the window should close
     *
     * @return true if the window close event has been triggered
     */
    bool shouldClose();

    /**
     * @brief Poll and process window events
     *
     * This should be called once per frame to handle user input
     * and window system events.
     */
    void pollEvents();

    // ========================================================================
    // GPU Resource Access
    // ========================================================================

    /**
     * @brief Get the WebGPU device
     *
     * @return The WebGPU device handle
     */
    wgpu::Device getDevice() const { return context_.getDevice(); }

    /**
     * @brief Get the WebGPU command queue
     *
     * @return The WebGPU queue handle
     */
    wgpu::Queue getQueue() const { return context_.getQueue(); }

    /**
     * @brief Get the surface texture format
     *
     * @return The texture format used by the rendering surface
     */
    wgpu::TextureFormat getSurfaceFormat() const { return context_.getSurfaceFormat(); }

    /**
     * @brief Get the rendering context
     *
     * @return Reference to the CeContext object
     */
    CeContext& getContext() { return context_; }

    /**
     * @brief Get the current GPU timer
     *
     * @return Shared pointer to the GPUTimer instance
     */
    std::shared_ptr<GPUTimer> getGPUTimer() const;

private:
    // ========================================================================
    // Internal Methods
    // ========================================================================

    /**
     * @brief Sort renderers by their layer order
     *
     * This is called automatically when renderers are added.
     */
    void sortRenderersByLayer();

    CeContext context_;                                      ///< WebGPU rendering context
    std::vector<std::unique_ptr<CeRenderable>> renderers_;  ///< Registered renderers
    float backgroundColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f};   ///< Background clear color (RGBA)
};
