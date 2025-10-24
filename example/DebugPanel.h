#pragma once

#include <webgpu/webgpu_cpp.h>
#include <clipengine/core/CompositionEngine.h>
#include <clipengine/effects/ShaderEffect.h>
#include <clipengine/layers/VideoRenderer.h>

struct GLFWwindow;
struct ImGuiContext;

/**
 * @brief ClipEngine Debug Panel
 *
 * Provides a comprehensive debugging interface for inspecting ClipEngine's internal state:
 * - Engine configuration and status
 * - Layer hierarchy and properties
 * - Filter chains (global and per-layer)
 * - Shader effect parameters with live editing
 *
 * Features:
 * - Tree-based hierarchical view
 * - Color-coded status indicators
 * - Real-time parameter adjustment
 * - Compact and organized layout
 */
class DebugPanel
{
public:
    DebugPanel() = default;
    ~DebugPanel();

    // Disable copy, allow move
    DebugPanel(const DebugPanel&) = delete;
    DebugPanel& operator=(const DebugPanel&) = delete;
    DebugPanel(DebugPanel&&) noexcept = default;
    DebugPanel& operator=(DebugPanel&&) noexcept = default;

    /**
     * @brief Initialize the debug panel
     * @param window GLFW window handle
     * @param device WebGPU device
     * @param format Surface texture format
     * @param engine Reference to the CompositionEngine to debug
     * @return true if initialization succeeded
     */
    bool initialize(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat format, CompositionEngine* engine);

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

    /**
     * @brief Begin a new frame
     */
    void beginFrame();

    /**
     * @brief End the frame and prepare for rendering
     */
    void endFrame();

    /**
     * @brief Render the debug UI
     * @param renderPass WebGPU render pass encoder
     */
    void render(void* renderPass);

    /**
     * @brief Toggle debug panel visibility
     */
    void toggleVisibility() { visible_ = !visible_; }

    /**
     * @brief Set debug panel visibility
     */
    void setVisible(bool visible) { visible_ = visible; }

    /**
     * @brief Check if debug panel is visible
     */
    bool isVisible() const { return visible_; }

    /**
     * @brief Render the main debug window
     */
    void renderDebugWindow();

private:
    void renderEngineInfo();
    void renderLayersTree();
    void renderGlobalFilters();
    void renderLayerFilters(size_t layerIndex);
    void renderShaderParams(ShaderEffect* effect, const char* name);
    void renderFilterChain(FilterChain* filterChain, const char* chainName);
    void renderColorAdjustmentTab();  // Color adjustment UI

    // Helper functions
    const char* getLayerTypeName(LayerType type);
    const char* getRenderModeName(VideoRenderer::RenderMode mode);
    void renderShaderParamValue(ShaderEffect* effect, const std::string& paramName, const ShaderParam* descriptor);
    void setupStyle();

    ImGuiContext* context_ = nullptr;  // Owned by DebugPanel
    GLFWwindow* window_ = nullptr;
    wgpu::Device device_;
    wgpu::TextureFormat format_;

    CompositionEngine* engine_ = nullptr;

    // UI state
    bool visible_ = true;
    bool initialized_ = false;

    // Selection state (for detailed parameter view)
    int selectedLayerIndex_ = -1;
    int selectedFilterIndex_ = -1;
};
