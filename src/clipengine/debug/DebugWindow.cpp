#include "DebugWindow.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>

#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <dawn/native/DawnNative.h>

#include "../effects/ShaderEffect.h"
#include "../layers/VideoRenderer.h"

#include <iostream>



DebugWindow::DebugWindow() = default;

DebugWindow::~DebugWindow()
{
    shutdown();
}

bool DebugWindow::initialize(CompositionEngine* engine, const char* title, int width, int height)
{
    if (initialized_) {
        std::cerr << "DebugWindow already initialized" << std::endl;
        return false;
    }

    if (!engine) {
        std::cerr << "CompositionEngine pointer is null" << std::endl;
        return false;
    }

    engine_ = engine;
    width_ = width;
    height_ = height;

    // Initialize GLFW if not already done
    static bool glfwInitialized = false;
    if (!glfwInitialized) {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW for debug window" << std::endl;
            return false;
        }
        glfwInitialized = true;
    }

    // Create debug window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(width_, height_, title, nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create debug window" << std::endl;
        return false;
    }

    // Reuse device from engine
    device_ = engine_->getDevice();
    format_ = engine_->getFormat();

    // Create surface for debug window using Dawn
#if defined(_WIN32)
    wgpu::SurfaceDescriptorFromWindowsHWND windowsDesc = {};
    windowsDesc.hwnd = glfwGetWin32Window(window_);
    windowsDesc.hinstance = GetModuleHandle(nullptr);

    wgpu::SurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &windowsDesc;

    // Get instance from device
    wgpu::Instance instance = device_.GetAdapter().GetInstance();
    surface_ = instance.CreateSurface(&surfaceDesc);
#endif

    if (!surface_) {
        std::cerr << "Failed to create surface for debug window" << std::endl;
        glfwDestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    // Configure surface
    wgpu::SurfaceConfiguration config = {};
    config.device = device_;
    config.format = format_;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.width = width_;
    config.height = height_;
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Opaque;

    surface_.Configure(&config);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    context_ = ImGui::CreateContext();
    if (!context_) {
        std::cerr << "Failed to create ImGui context for debug window" << std::endl;
        shutdown();
        return false;
    }

    ImGui::SetCurrentContext(context_);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    setupStyle();

    // Setup backends
    if (!ImGui_ImplGlfw_InitForOther(window_, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        shutdown();
        return false;
    }

    ImGui_ImplWGPU_InitInfo init_info = {};
    init_info.Device = device_.Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(format_);
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;

    if (!ImGui_ImplWGPU_Init(&init_info)) {
        std::cerr << "Failed to initialize ImGui WebGPU backend" << std::endl;
        shutdown();
        return false;
    }

    initialized_ = true;
    std::cout << "DebugWindow initialized successfully" << std::endl;

    return true;
}

void DebugWindow::shutdown()
{
    if (!initialized_) {
        return;
    }

    if (context_) {
        ImGui::SetCurrentContext(context_);
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(context_);
        context_ = nullptr;
    }

    if (surface_) {
        surface_ = nullptr;
    }

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    initialized_ = false;
}

void DebugWindow::update()
{
    if (!initialized_ || !window_) {
        return;
    }

    // Poll events
    glfwPollEvents();

    // Check if window is minimized
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    if (display_w == 0 || display_h == 0) {
        return;
    }

    // Begin ImGui frame
    ImGui::SetCurrentContext(context_);
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render UI
    renderUI();

    // Render ImGui
    ImGui::Render();

    // Get surface texture
    wgpu::SurfaceTexture surfaceTexture;
    surface_.GetCurrentTexture(&surfaceTexture);

    if (!surfaceTexture.texture) {
        std::cerr << "Failed to get current texture for debug window" << std::endl;
        return;
    }

    wgpu::TextureView view = surfaceTexture.texture.CreateView();

    // Render pass
    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = view;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {0.1f, 0.1f, 0.12f, 1.0f};

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    device_.GetQueue().Submit(1, &commands);

    // Present
    surface_.Present();
}

bool DebugWindow::shouldClose() const
{
    return window_ && glfwWindowShouldClose(window_);
}

void DebugWindow::setVisible(bool visible)
{
    if (!window_) return;

    if (visible) {
        glfwShowWindow(window_);
    } else {
        glfwHideWindow(window_);
    }
}

bool DebugWindow::isVisible() const
{
    if (!window_) return false;
    return glfwGetWindowAttrib(window_, GLFW_VISIBLE) != 0;
}

void DebugWindow::renderUI()
{
    if (!engine_) return;

    // Full window debug panel
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::Begin("ClipEngine Debug", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Engine Debugger");
    ImGui::Separator();

    // Tabs
    if (ImGui::BeginTabBar("DebugTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            renderEngineInfo();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Layers")) {
            renderLayersTree();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Global Filters")) {
            renderGlobalFilters();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Color Adjust")) {
            renderColorAdjustmentTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// Copy implementation from DebugPanel.cpp
void DebugWindow::renderEngineInfo()
{
    if (!engine_) return;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Engine Configuration");
    ImGui::Separator();

    const char* formatName = "Unknown";
    wgpu::TextureFormat format = engine_->getFormat();
    switch (format) {
        case wgpu::TextureFormat::BGRA8Unorm: formatName = "BGRA8Unorm"; break;
        case wgpu::TextureFormat::RGBA8Unorm: formatName = "RGBA8Unorm"; break;
        case wgpu::TextureFormat::RGBA16Float: formatName = "RGBA16Float"; break;
        default: break;
    }

    ImGui::Text("Surface Format: %s", formatName);
    ImGui::Text("Backend: WebGPU (Dawn)");

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Composition");
    ImGui::Separator();

    size_t layerCount = engine_->getLayerCount();
    ImGui::Text("Total Layers: %zu", layerCount);

    size_t globalFilterCount = engine_->getGlobalFilterChain().getFilterCount();
    ImGui::Text("Global Filters: %zu", globalFilterCount);

    size_t enabledLayers = 0;
    for (size_t i = 0; i < layerCount; ++i) {
        CompositionLayer* layer = engine_->getLayer(i);
        if (layer && layer->isEnabled()) {
            ++enabledLayers;
        }
    }
    ImGui::Text("Enabled Layers: %zu / %zu", enabledLayers, layerCount);
}

void DebugWindow::renderLayersTree()
{
    if (!engine_) return;

    ImGui::Spacing();

    size_t layerCount = engine_->getLayerCount();
    if (layerCount == 0) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No layers in composition");
        return;
    }

    for (size_t i = 0; i < layerCount; ++i) {
        CompositionLayer* layer = engine_->getLayer(i);
        if (!layer) continue;

        ImGui::PushID((int)i);

        bool enabled = layer->isEnabled();
        ImVec4 headerColor = enabled ? ImVec4(0.4f, 0.8f, 0.4f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

        const char* typeName = getLayerTypeName(layer->getType());
        const char* layerName = layer->getName().empty() ? "Unnamed" : layer->getName().c_str();

        ImGui::PushStyleColor(ImGuiCol_Text, headerColor);
        bool nodeOpen = ImGui::TreeNode("LayerNode", "[%d] %s (%s)", layer->getLayer(), layerName, typeName);
        ImGui::PopStyleColor();

        if (nodeOpen) {
            ImGui::Indent();

            ImGui::Checkbox("Enabled", &enabled);
            layer->setEnabled(enabled);

            ImGui::Text("Type: %s", typeName);
            ImGui::Text("Z-Order: %d", layer->getLayer());
            ImGui::Text("Name: %s", layerName);

            const LayerTransform& transform = layer->getTransform();
            ImGui::Text("Transform:");
            ImGui::Indent();
            ImGui::Text("Position: (%.2f, %.2f)", transform.x, transform.y);
            ImGui::Text("Size: (%.2f, %.2f)", transform.width, transform.height);
            ImGui::Unindent();

            if (layer->getType() == LayerType::Video) {
                VideoRenderer* video = static_cast<VideoRenderer*>(layer);
                ImGui::Text("Render Mode: %s", getRenderModeName(video->getRenderMode()));
            }

            ImGui::Spacing();
            renderLayerFilters(i);

            ImGui::Unindent();
            ImGui::TreePop();
        }

        ImGui::PopID();
        ImGui::Spacing();
    }
}

void DebugWindow::renderGlobalFilters()
{
    if (!engine_) return;

    ImGui::Spacing();
    FilterChain& globalChain = engine_->getGlobalFilterChain();
    renderFilterChain(&globalChain, "Global Filter Chain");
}

void DebugWindow::renderLayerFilters(size_t layerIndex)
{
    if (!engine_) return;

    FilterChain* filterChain = engine_->getLayerFilterChain(layerIndex);
    if (!filterChain) return;

    renderFilterChain(filterChain, "Layer Filters");
}

void DebugWindow::renderFilterChain(FilterChain* filterChain, const char* chainName)
{
    if (!filterChain) return;

    size_t filterCount = filterChain->getFilterCount();

    if (ImGui::TreeNode(chainName, "%s (%zu)", chainName, filterCount)) {
        if (filterCount == 0) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No filters");
        } else {
            for (size_t i = 0; i < filterCount; ++i) {
                Filter* filter = filterChain->getFilter(i);
                if (!filter) continue;

                ImGui::PushID((int)i);

                ShaderEffect* effect = dynamic_cast<ShaderEffect*>(filter);
                if (effect) {
                    char filterName[64];
                    snprintf(filterName, sizeof(filterName), "Filter %zu (ShaderEffect)", i);
                    renderShaderParams(effect, filterName);
                } else {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "Filter %zu (Unknown Type)", i);
                }

                ImGui::PopID();
            }
        }
        ImGui::TreePop();
    }
}

void DebugWindow::renderShaderParams(ShaderEffect* effect, const char* name)
{
    if (!effect) return;

    if (ImGui::TreeNode(name)) {
        std::vector<std::string> paramNames = effect->getParamNames();

        if (paramNames.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No parameters");
        } else {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Parameters:");
            ImGui::Separator();

            for (const auto& paramName : paramNames) {
                const ShaderParam* descriptor = effect->getParamDescriptor(paramName);
                renderShaderParamValue(effect, paramName, descriptor);
            }
        }

        ImGui::TreePop();
    }
}

void DebugWindow::renderShaderParamValue(ShaderEffect* effect, const std::string& paramName, const ShaderParam* descriptor)
{
    if (!effect) return;

    ShaderParamValue value = effect->getParam(paramName);

    ImGui::PushID(paramName.c_str());

    float minVal = -10.0f, maxVal = 10.0f;
    if (descriptor) {
        if (std::holds_alternative<float>(descriptor->minValue)) {
            minVal = std::get<float>(descriptor->minValue);
        }
        if (std::holds_alternative<float>(descriptor->maxValue)) {
            maxVal = std::get<float>(descriptor->maxValue);
        }
    }

    if (std::holds_alternative<float>(value)) {
        float v = std::get<float>(value);
        if (ImGui::SliderFloat(paramName.c_str(), &v, minVal, maxVal)) {
            effect->setParam(paramName, v);
        }
        if (descriptor && !descriptor->description.empty()) {
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", descriptor->description.c_str());
            }
        }
    }
    else if (std::holds_alternative<int>(value)) {
        int v = std::get<int>(value);
        if (ImGui::SliderInt(paramName.c_str(), &v, (int)minVal, (int)maxVal)) {
            effect->setParam(paramName, v);
        }
    }
    else if (std::holds_alternative<bool>(value)) {
        bool v = std::get<bool>(value);
        if (ImGui::Checkbox(paramName.c_str(), &v)) {
            effect->setParam(paramName, v);
        }
    }
    else if (std::holds_alternative<std::array<float, 2>>(value)) {
        auto v = std::get<std::array<float, 2>>(value);
        if (ImGui::SliderFloat2(paramName.c_str(), v.data(), minVal, maxVal)) {
            effect->setParam(paramName, v);
        }
    }
    else if (std::holds_alternative<std::array<float, 3>>(value)) {
        auto v = std::get<std::array<float, 3>>(value);
        if (ImGui::ColorEdit3(paramName.c_str(), v.data())) {
            effect->setParam(paramName, v);
        }
    }
    else if (std::holds_alternative<std::array<float, 4>>(value)) {
        auto v = std::get<std::array<float, 4>>(value);
        if (ImGui::ColorEdit4(paramName.c_str(), v.data())) {
            effect->setParam(paramName, v);
        }
    }

    ImGui::PopID();
}

void DebugWindow::renderColorAdjustmentTab()
{
    if (!engine_) return;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Quick Color Adjustment");
    ImGui::Separator();

    FilterChain& globalChain = engine_->getGlobalFilterChain();
    ShaderEffect* colorEffect = nullptr;

    for (size_t i = 0; i < globalChain.getFilterCount(); ++i) {
        Filter* filter = globalChain.getFilter(i);
        ShaderEffect* effect = dynamic_cast<ShaderEffect*>(filter);
        if (effect) {
            auto params = effect->getParamNames();
            bool hasColorParams = false;
            for (const auto& name : params) {
                if (name == "brightness" || name == "contrast" || name == "saturation") {
                    hasColorParams = true;
                    break;
                }
            }
            if (hasColorParams) {
                colorEffect = effect;
                break;
            }
        }
    }

    if (colorEffect) {
        ImGui::Text("Global Color Adjustment Effect Found");
        ImGui::Spacing();
        renderShaderParams(colorEffect, "Parameters");
    } else {
        ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.4f, 1.0f), "No color adjustment effect found");
        ImGui::Text("Add a ColorAdjust effect to the global filter chain");
    }
}

void DebugWindow::setupStyle()
{
    ImGui::SetCurrentContext(context_);

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FrameRounding = 4.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.GrabRounding = 3.0f;
    style.ScrollbarSize = 14.0f;
    style.ScrollbarRounding = 4.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
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
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.97f, 1.0f);
}

const char* DebugWindow::getLayerTypeName(LayerType type)
{
    switch (type) {
        case LayerType::Video: return "Video";
        case LayerType::Image: return "Image";
        case LayerType::Text: return "Text";
        case LayerType::Shape: return "Shape";
        case LayerType::Audio: return "Audio";
        case LayerType::Effect: return "Effect";
        case LayerType::Adjustment: return "Adjustment";
        default: return "Unknown";
    }
}

const char* DebugWindow::getRenderModeName(VideoRenderer::RenderMode mode)
{
    switch (mode) {
        case VideoRenderer::RenderMode::Planar: return "Planar";
        case VideoRenderer::RenderMode::Panorama: return "Panorama";
        default: return "Unknown";
    }
}


