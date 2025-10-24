#include "UIPanel.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

#include <clipengine/effects/ShaderEffect.h>

#include <iostream>

UIPanel::~UIPanel()
{
    shutdown();
}

bool UIPanel::initialize(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat format)
{
    if (initialized_) {
        std::cerr << "UIPanel already initialized" << std::endl;
        return false;
    }

    window_ = window;
    device_ = device;
    format_ = format;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    context_ = ImGui::CreateContext();
    if (!context_) {
        std::cerr << "Failed to create ImGui context" << std::endl;
        return false;
    }

    ImGui::SetCurrentContext(context_);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls

    // Setup style
    setupStyle();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOther(window_, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        ImGui::DestroyContext(context_);
        context_ = nullptr;
        return false;
    }

    // Setup WebGPU backend
    ImGui_ImplWGPU_InitInfo init_info = {};
    init_info.Device = device.Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(format);
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;

    if (!ImGui_ImplWGPU_Init(&init_info)) {
        std::cerr << "Failed to initialize ImGui WebGPU backend" << std::endl;
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(context_);
        context_ = nullptr;
        return false;
    }

    initialized_ = true;
    std::cout << "UIPanel initialized successfully" << std::endl;

    return true;
}

void UIPanel::shutdown()
{
    if (!initialized_) {
        return;
    }

    ImGui::SetCurrentContext(context_);

    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    if (context_) {
        ImGui::DestroyContext(context_);
        context_ = nullptr;
    }

    initialized_ = false;
}

void UIPanel::beginFrame()
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIPanel::endFrame()
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);
    ImGui::Render();
}

void UIPanel::render(void* renderPass)
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), static_cast<WGPURenderPassEncoder>(renderPass));
}

void UIPanel::renderColorAdjustment(ShaderEffect* effect)
{
    if (!initialized_ || !visible_) {
        return;
    }

    if (effect) {
        shaderEffect_ = effect;
    }

    ImGui::SetCurrentContext(context_);

    // Main control panel
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_FirstUseEver);

    ImGui::Begin("Color Adjustment", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Video Color Controls");
    ImGui::Separator();

    bool changed = false;

    // Basic adjustments
    ImGui::Text("Basic Adjustments");
    changed |= ImGui::SliderFloat("Exposure", &exposure_, -1.0f, 1.0f, "%.2f");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Adjust overall brightness (EV)");
    }

    changed |= ImGui::SliderFloat("Contrast", &contrast_, 0.0f, 2.0f, "%.2f");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Adjust contrast ratio");
    }

    changed |= ImGui::SliderFloat("Saturation", &saturation_, 0.0f, 2.0f, "%.2f");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Adjust color saturation");
    }

    changed |= ImGui::SliderFloat("Brightness", &brightness_, -1.0f, 1.0f, "%.2f");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Adjust brightness offset");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Advanced adjustments
    if (ImGui::CollapsingHeader("Advanced", ImGuiTreeNodeFlags_None)) {
        ImGui::Indent();

        changed |= ImGui::SliderFloat("Hue", &hue_, -180.0f, 180.0f, "%.0f°");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Rotate hue (degrees)");
        }

        changed |= ImGui::SliderFloat("Gamma", &gamma_, 0.5f, 2.5f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust gamma correction");
        }

        changed |= ImGui::SliderFloat("Temperature", &temperature_, -1.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Cool (blue) to Warm (orange)");
        }

        changed |= ImGui::SliderFloat("Tint", &tint_, -1.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Green to Magenta");
        }

        ImGui::Unindent();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Reset button
    if (ImGui::Button("Reset All", ImVec2(-1, 0))) {
        resetParameters();
        changed = true;
    }

    // Update shader parameters when changed
    if (changed) {
        updateShaderParameters();
    }

    // Info section
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Press F1 to toggle UI");

    ImGui::End();
}

void UIPanel::resetParameters()
{
    exposure_ = 0.0f;
    contrast_ = 1.0f;
    saturation_ = 1.0f;
    brightness_ = 0.0f;
    hue_ = 0.0f;
    gamma_ = 1.0f;
    temperature_ = 0.0f;
    tint_ = 0.0f;

    updateShaderParameters();
}

void UIPanel::setupStyle()
{
    ImGui::SetCurrentContext(context_);

    // Professional dark theme
    ImGuiStyle& style = ImGui::GetStyle();

    // Window
    style.WindowRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    // Frame
    style.FrameRounding = 4.0f;
    style.FramePadding = ImVec2(8, 4);
    style.FrameBorderSize = 0.0f;

    // Items
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);

    // Grab
    style.GrabRounding = 3.0f;
    style.GrabMinSize = 12.0f;

    // Scrollbar
    style.ScrollbarSize = 14.0f;
    style.ScrollbarRounding = 4.0f;

    // Colors - Modern dark theme
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 0.75f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.50f, 0.65f, 1.00f);

    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.60f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.70f, 0.95f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.40f, 0.55f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.45f, 0.65f, 1.00f);

    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.28f, 0.60f);
    colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);

    colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.75f, 1.00f, 1.00f);

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
}

void UIPanel::updateShaderParameters()
{
    if (!shaderEffect_) {
        return;
    }

    // Map UI parameters to shader parameters
    // Exposure is mapped to brightness in the shader
    shaderEffect_->setParam("brightness", exposure_);
    shaderEffect_->setParam("contrast", contrast_);
    shaderEffect_->setParam("saturation", saturation_);

    // Advanced parameters (if the shader supports them)
    // Note: The basic colorAdjust shader may not support all parameters
    // You can extend the shader to support these in the future
}

void UIPanel::updatePerformance(float deltaTime)
{
    frameTime_ = deltaTime * 1000.0f; // Convert to milliseconds
    currentFPS_ = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;

    // Update frame history for graph
    frameHistory_[frameHistoryIndex_] = frameTime_;
    frameHistoryIndex_ = (frameHistoryIndex_ + 1) % FRAME_HISTORY_SIZE;

    // Update statistics
    accumulatedTime_ += deltaTime;
    frameCount_++;

    if (accumulatedTime_ >= 1.0f) {
        averageFPS_ = frameCount_ / accumulatedTime_;
        accumulatedTime_ = 0.0f;
        frameCount_ = 0;
    }

    if (currentFPS_ > maxFPS_) maxFPS_ = currentFPS_;
    if (currentFPS_ < minFPS_ && currentFPS_ > 0.0f) minFPS_ = currentFPS_;
}

void UIPanel::setVideoInfo(int width, int height, const char* codec, float fps)
{
    videoWidth_ = width;
    videoHeight_ = height;
    videoFPS_ = fps;
    if (codec) {
        strncpy_s(videoCodec_, sizeof(videoCodec_), codec, _TRUNCATE);
    }
}

void UIPanel::renderPerformancePanel()
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);

    ImGui::Begin("Performance Monitor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Frame Statistics");
    ImGui::Separator();

    // FPS Display
    ImGui::Text("FPS: %.1f", currentFPS_);
    ImGui::SameLine(150);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Avg: %.1f", averageFPS_);

    ImGui::Text("Frame Time: %.2f ms", frameTime_);
    ImGui::Text("Min FPS: %.1f", minFPS_);
    ImGui::SameLine(150);
    ImGui::Text("Max FPS: %.1f", maxFPS_);

    // Frame time graph
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Frame Time Graph");
    ImGui::PlotLines("##frametime", frameHistory_, FRAME_HISTORY_SIZE, frameHistoryIndex_,
        nullptr, 0.0f, 50.0f, ImVec2(0, 80));

    ImGui::Spacing();
    if (ImGui::Button("Reset Stats", ImVec2(-1, 0))) {
        minFPS_ = 999.0f;
        maxFPS_ = 0.0f;
        averageFPS_ = 0.0f;
        accumulatedTime_ = 0.0f;
        frameCount_ = 0;
    }

    ImGui::End();
}

void UIPanel::renderSystemInfo()
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);

    ImGui::SetNextWindowPos(ImVec2(360, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);

    ImGui::Begin("System Information", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Render Configuration");
    ImGui::Separator();

    ImGui::Text("Resolution: %d x %d", renderWidth_, renderHeight_);

    // Format name
    const char* formatName = "Unknown";
    switch (format_) {
        case wgpu::TextureFormat::BGRA8Unorm: formatName = "BGRA8Unorm"; break;
        case wgpu::TextureFormat::RGBA8Unorm: formatName = "RGBA8Unorm"; break;
        case wgpu::TextureFormat::RGBA16Float: formatName = "RGBA16Float"; break;
        default: break;
    }
    ImGui::Text("Format: %s", formatName);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "WebGPU Device");
    ImGui::Separator();

    ImGui::Text("Backend: Dawn");
    ImGui::Text("API: WebGPU");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Press F1 to toggle UI");

    ImGui::End();
}

void UIPanel::renderVideoInfo()
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);

    ImGui::SetNextWindowPos(ImVec2(730, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);

    ImGui::Begin("Video Information", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Video Stream");
    ImGui::Separator();

    ImGui::Text("Resolution: %d x %d", videoWidth_, videoHeight_);
    ImGui::Text("Codec: %s", videoCodec_);
    ImGui::Text("FPS: %.2f", videoFPS_);

    float aspectRatio = videoHeight_ > 0 ? (float)videoWidth_ / (float)videoHeight_ : 0.0f;
    ImGui::Text("Aspect Ratio: %.2f:1", aspectRatio);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Render Mode");
    ImGui::Separator();
    ImGui::Text("Mode: 360° Panorama");

    ImGui::End();
}

void UIPanel::renderDebugUI()
{
    if (!initialized_ || !visible_) {
        return;
    }

    renderPerformancePanel();
    renderSystemInfo();
    renderVideoInfo();
    renderColorAdjustment();
}
