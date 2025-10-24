#include "DebugPanel.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>

#include <GLFW/glfw3.h>
#include <clipengine/effects/ShaderEffect.h>
#include <clipengine/layers/VideoRenderer.h>

#include <iostream>

DebugPanel::~DebugPanel()
{
    shutdown();
}

bool DebugPanel::initialize(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat format, CompositionEngine* engine)
{
    if (initialized_) {
        std::cerr << "DebugPanel already initialized" << std::endl;
        return false;
    }

    if (!engine) {
        std::cerr << "CompositionEngine pointer is null" << std::endl;
        return false;
    }

    window_ = window;
    device_ = device;
    format_ = format;
    engine_ = engine;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    context_ = ImGui::CreateContext();
    if (!context_) {
        std::cerr << "Failed to create ImGui context" << std::endl;
        return false;
    }

    ImGui::SetCurrentContext(context_);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup style
    setupStyle();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOther(window_, true)) {
        std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
        ImGui::DestroyContext(context_);
        context_ = nullptr;
        return false;
    }

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
    std::cout << "DebugPanel initialized successfully" << std::endl;

    return true;
}

void DebugPanel::shutdown()
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

void DebugPanel::beginFrame()
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugPanel::endFrame()
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);
    ImGui::Render();
}

void DebugPanel::render(void* renderPass)
{
    if (!initialized_ || !visible_) {
        return;
    }

    ImGui::SetCurrentContext(context_);
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), static_cast<WGPURenderPassEncoder>(renderPass));
}

void DebugPanel::renderDebugWindow()
{
    if (!initialized_ || !visible_ || !engine_) {
        return;
    }

    ImGui::SetCurrentContext(context_);

    // Main debug window
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(480, 700), ImGuiCond_FirstUseEver);

    ImGui::Begin("ClipEngine Debug", nullptr);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Engine Debugger");
    ImGui::Separator();

    // Tabs for different debug views
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

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Press F1 to toggle debug panel");

    ImGui::End();
}

void DebugPanel::renderEngineInfo()
{
    if (!engine_) return;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Engine Configuration");
    ImGui::Separator();

    // Get format name
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

    // Count enabled layers
    size_t enabledLayers = 0;
    for (size_t i = 0; i < layerCount; ++i) {
        CompositionLayer* layer = engine_->getLayer(i);
        if (layer && layer->isEnabled()) {
            ++enabledLayers;
        }
    }
    ImGui::Text("Enabled Layers: %zu / %zu", enabledLayers, layerCount);
}

void DebugPanel::renderLayersTree()
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

        // Layer header with enabled checkbox
        bool enabled = layer->isEnabled();
        ImVec4 headerColor = enabled ? ImVec4(0.4f, 0.8f, 0.4f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

        // Layer tree node
        const char* typeName = getLayerTypeName(layer->getType());
        const char* layerName = layer->getName().empty() ? "Unnamed" : layer->getName().c_str();

        ImGui::PushStyleColor(ImGuiCol_Text, headerColor);
        bool nodeOpen = ImGui::TreeNode("LayerNode", "[%d] %s (%s)", layer->getLayer(), layerName, typeName);
        ImGui::PopStyleColor();

        if (nodeOpen) {
            ImGui::Indent();

            // Enabled checkbox
            ImGui::Checkbox("Enabled", &enabled);
            layer->setEnabled(enabled);

            // Layer properties
            ImGui::Text("Type: %s", typeName);
            ImGui::Text("Z-Order: %d", layer->getLayer());
            ImGui::Text("Name: %s", layerName);

            // Transform
            const LayerTransform& transform = layer->getTransform();
            ImGui::Text("Transform:");
            ImGui::Indent();
            ImGui::Text("Position: (%.2f, %.2f)", transform.x, transform.y);
            ImGui::Text("Size: (%.2f, %.2f)", transform.width, transform.height);
            ImGui::Unindent();

            // Video-specific info
            if (layer->getType() == LayerType::Video) {
                VideoRenderer* video = static_cast<VideoRenderer*>(layer);
                ImGui::Text("Render Mode: %s", getRenderModeName(video->getRenderMode()));
            }

            // Layer filters
            ImGui::Spacing();
            renderLayerFilters(i);

            ImGui::Unindent();
            ImGui::TreePop();
        }

        ImGui::PopID();
        ImGui::Spacing();
    }
}

void DebugPanel::renderGlobalFilters()
{
    if (!engine_) return;

    ImGui::Spacing();
    FilterChain& globalChain = engine_->getGlobalFilterChain();
    renderFilterChain(&globalChain, "Global Filter Chain");
}

void DebugPanel::renderLayerFilters(size_t layerIndex)
{
    if (!engine_) return;

    FilterChain* filterChain = engine_->getLayerFilterChain(layerIndex);
    if (!filterChain) return;

    renderFilterChain(filterChain, "Layer Filters");
}

void DebugPanel::renderFilterChain(FilterChain* filterChain, const char* chainName)
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

                // Try to cast to ShaderEffect to get more info
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

void DebugPanel::renderShaderParams(ShaderEffect* effect, const char* name)
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

void DebugPanel::renderShaderParamValue(ShaderEffect* effect, const std::string& paramName, const ShaderParam* descriptor)
{
    if (!effect) return;

    ShaderParamValue value = effect->getParam(paramName);

    ImGui::PushID(paramName.c_str());

    // Get min/max values from descriptor if available
    float minVal = -10.0f, maxVal = 10.0f;
    if (descriptor) {
        if (std::holds_alternative<float>(descriptor->minValue)) {
            minVal = std::get<float>(descriptor->minValue);
        }
        if (std::holds_alternative<float>(descriptor->maxValue)) {
            maxVal = std::get<float>(descriptor->maxValue);
        }
    }

    // Render based on type
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

const char* DebugPanel::getLayerTypeName(LayerType type)
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

const char* DebugPanel::getRenderModeName(VideoRenderer::RenderMode mode)
{
    switch (mode) {
        case VideoRenderer::RenderMode::Planar: return "Planar";
        case VideoRenderer::RenderMode::Panorama: return "Panorama";
        default: return "Unknown";
    }
}

void DebugPanel::setupStyle()
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

    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.30f, 0.40f, 1.00f);
}

void DebugPanel::renderColorAdjustmentTab()
{
    if (!engine_) return;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Quick Color Adjustment");
    ImGui::Separator();

    // Find the color adjustment effect in global filter chain
    FilterChain& globalChain = engine_->getGlobalFilterChain();
    ShaderEffect* colorEffect = nullptr;

    for (size_t i = 0; i < globalChain.getFilterCount(); ++i) {
        Filter* filter = globalChain.getFilter(i);
        ShaderEffect* effect = dynamic_cast<ShaderEffect*>(filter);
        if (effect) {
            // Check if this has brightness/contrast/saturation params
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
