#pragma once

#include <clipengine/utils/Common.h>
#include <clipengine/core/CompositionEngine.h>
#include <clipengine/effects/ShaderEffect.h>
#include <clipengine/layers/VideoRenderer.h>

struct GLFWwindow;
struct ImGuiContext;

/**
 * @brief Standalone debug window for ClipEngine
 *
 * Provides a separate window with comprehensive debugging interface:
 * - Engine configuration and status
 * - Layer hierarchy and properties
 * - Filter chains (global and per-layer)
 * - Shader effect parameters with live editing
 * - Color adjustment controls
 *
 * Features:
 * - Runs in independent window
 * - Non-blocking update (doesn't affect main render loop)
 * - Can be placed on second monitor
 * - Optional - easy to enable/disable
 *
 * Usage:
 * @code
 * CompositionEngine engine;
 * engine.initialize(...);
 *
 * DebugWindow debugWindow;
 * debugWindow.initialize(&engine, "ClipEngine Debug");
 *
 * // In main loop
 * while (!shouldClose) {
 *     engine.render(...);
 *     debugWindow.update();  // Non-blocking
 * }
 *
 * debugWindow.shutdown();
 * @endcode
 */
class DebugWindow
{
public:
    DebugWindow();
    ~DebugWindow();

    // Disable copy, allow move
    DebugWindow(const DebugWindow&) = delete;
    DebugWindow& operator=(const DebugWindow&) = delete;
    DebugWindow(DebugWindow&&) noexcept = default;
    DebugWindow& operator=(DebugWindow&&) noexcept = default;

    /**
     * @brief Initialize the debug window
     * @param engine CompositionEngine to debug
     * @param title Window title (default: "ClipEngine Debug")
     * @param width Window width (default: 600)
     * @param height Window height (default: 800)
     * @return true if initialization succeeded
     */
    bool initialize(CompositionEngine* engine,
                   const char* title = "ClipEngine Debug",
                   int width = 600,
                   int height = 800);

    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();

    /**
     * @brief Update the debug window (call once per frame)
     * This is non-blocking and won't affect main render loop
     */
    void update();

    /**
     * @brief Check if window should close
     */
    bool shouldClose() const;

    /**
     * @brief Check if debug window is initialized
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Set window visibility
     */
    void setVisible(bool visible);

    /**
     * @brief Check if window is visible
     */
    bool isVisible() const;

private:
    void renderUI();
    void renderEngineInfo();
    void renderLayersTree();
    void renderGlobalFilters();
    void renderLayerFilters(size_t layerIndex);
    void renderShaderParams(ShaderEffect* effect, const char* name);
    void renderFilterChain(FilterChain* filterChain, const char* chainName);
    void renderColorAdjustmentTab();
    void renderShaderParamValue(ShaderEffect* effect, const std::string& paramName, const ShaderParam* descriptor);

    void setupStyle();
    const char* getLayerTypeName(LayerType type);
    const char* getRenderModeName(VideoRenderer::RenderMode mode);

    GLFWwindow* window_ = nullptr;
    ImGuiContext* context_ = nullptr;

    wgpu::Device device_;
    wgpu::Surface surface_;
    wgpu::TextureFormat format_ = wgpu::TextureFormat::BGRA8Unorm;

    CompositionEngine* engine_ = nullptr;

    bool initialized_ = false;
    int width_ = 600;
    int height_ = 800;
};

