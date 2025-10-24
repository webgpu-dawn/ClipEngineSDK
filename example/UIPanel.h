#pragma once

#include <string>
#include <functional>
#include <memory>
#include <webgpu/webgpu_cpp.h>

struct GLFWwindow;
struct ImGuiContext;

class ShaderEffect;

/**
 * @brief Professional Debug UI Panel
 *
 * This module provides a comprehensive debugging interface for the application,
 * combining shader parameter adjustment with real-time performance monitoring
 * and system information display.
 *
 * Features:
 * - Real-time performance monitoring (FPS, frame time)
 * - System and render information display
 * - Video stream information
 * - Shader parameter adjustment
 * - Professional dark theme styling
 * - Self-contained ImGui lifecycle management
 */
class UIPanel
{
public:
    UIPanel() = default;
    ~UIPanel();

    // Disable copy, allow move
    UIPanel(const UIPanel&) = delete;
    UIPanel& operator=(const UIPanel&) = delete;
    UIPanel(UIPanel&&) noexcept = default;
    UIPanel& operator=(UIPanel&&) noexcept = default;

    /**
     * @brief Initialize the UI panel
     * @param window GLFW window handle
     * @param device WebGPU device
     * @param format Surface texture format
     * @return true if initialization succeeded
     */
    bool initialize(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat format);

    /**
     * @brief Shutdown and cleanup UI resources
     */
    void shutdown();

    /**
     * @brief Begin a new frame (call before rendering UI)
     */
    void beginFrame();

    /**
     * @brief End the frame and prepare for rendering (call after UI code)
     */
    void endFrame();

    /**
     * @brief Render the UI (call during render pass)
     * @param renderPass WebGPU render pass encoder
     */
    void render(void* renderPass);

    /**
     * @brief Toggle UI visibility
     */
    void toggleVisibility() { visible_ = !visible_; }

    /**
     * @brief Set UI visibility
     */
    void setVisible(bool visible) { visible_ = visible; }

    /**
     * @brief Check if UI is visible
     */
    bool isVisible() const { return visible_; }

    /**
     * @brief Render color adjustment panel
     * @param effect Shader effect to control (optional, can be set later)
     */
    void renderColorAdjustment(ShaderEffect* effect = nullptr);

    /**
     * @brief Render performance monitoring panel
     */
    void renderPerformancePanel();

    /**
     * @brief Render system information panel
     */
    void renderSystemInfo();

    /**
     * @brief Render video information panel
     */
    void renderVideoInfo();

    /**
     * @brief Render all debug panels
     */
    void renderDebugUI();

    /**
     * @brief Set the shader effect to control
     */
    void setShaderEffect(ShaderEffect* effect) { shaderEffect_ = effect; }

    /**
     * @brief Update performance metrics (call once per frame)
     * @param deltaTime Frame delta time in seconds
     */
    void updatePerformance(float deltaTime);

    /**
     * @brief Set video information
     */
    void setVideoInfo(int width, int height, const char* codec, float fps);

    /**
     * @brief Set render resolution
     */
    void setRenderResolution(int width, int height) {
        renderWidth_ = width;
        renderHeight_ = height;
    }

    /**
     * @brief Get current exposure value
     */
    float getExposure() const { return exposure_; }

    /**
     * @brief Get current contrast value
     */
    float getContrast() const { return contrast_; }

    /**
     * @brief Get current saturation value
     */
    float getSaturation() const { return saturation_; }

    /**
     * @brief Reset all parameters to defaults
     */
    void resetParameters();

    /**
     * @brief Get the ImGui context for sharing with other panels
     */
    ImGuiContext* getContext() const { return context_; }

private:
    void setupStyle();
    void updateShaderParameters();

    ImGuiContext* context_ = nullptr;
    GLFWwindow* window_ = nullptr;

    ShaderEffect* shaderEffect_ = nullptr;

    // UI state
    bool visible_ = true;
    bool initialized_ = false;

    // Color adjustment parameters
    float exposure_ = 0.0f;      // Range: -1.0 to 1.0
    float contrast_ = 1.0f;      // Range: 0.0 to 2.0
    float saturation_ = 1.0f;    // Range: 0.0 to 2.0
    float brightness_ = 0.0f;    // Range: -1.0 to 1.0
    float hue_ = 0.0f;           // Range: -180 to 180 degrees

    // Advanced parameters
    float gamma_ = 1.0f;         // Range: 0.5 to 2.5
    float temperature_ = 0.0f;   // Range: -1.0 to 1.0 (cool to warm)
    float tint_ = 0.0f;          // Range: -1.0 to 1.0 (green to magenta)

    // Performance monitoring
    static constexpr int FRAME_HISTORY_SIZE = 120;
    float frameHistory_[FRAME_HISTORY_SIZE] = {};
    int frameHistoryIndex_ = 0;
    float currentFPS_ = 0.0f;
    float averageFPS_ = 0.0f;
    float minFPS_ = 999.0f;
    float maxFPS_ = 0.0f;
    float frameTime_ = 0.0f;
    float accumulatedTime_ = 0.0f;
    int frameCount_ = 0;

    // Video information
    int videoWidth_ = 0;
    int videoHeight_ = 0;
    char videoCodec_[32] = "Unknown";
    float videoFPS_ = 0.0f;

    // Render information
    int renderWidth_ = 0;
    int renderHeight_ = 0;
    wgpu::Device device_;
    wgpu::TextureFormat format_;
};
