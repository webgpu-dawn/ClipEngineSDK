#include "CompositionEngine.h"
#include "../layers/TextureRenderer.h"
#ifdef CLIPENGINE_DEBUG_WINDOW_ENABLED
#include "../debug/DebugWindow.h"
#endif
#include <iostream>

// Explicit constructor/destructor definition (needed for unique_ptr with incomplete type)
CompositionEngine::CompositionEngine() = default;
CompositionEngine::~CompositionEngine() = default;

bool CompositionEngine::initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height) {
    device_ = device;
    format_ = format;
    width_ = width;
    height_ = height;
    ownsContext_ = false;  // Using external device

    // Initialize input state
    inputState_.setResolution(width_, height_);

    // Initialize global filter chain
    globalFilterChain_.initialize(device_, format_, width_, height_);

    createIntermediateTextures();

#ifdef CLIPENGINE_DEBUG_WINDOW_ENABLED
    // Automatically create debug window if enabled
    debugWindow_ = std::make_unique<DebugWindow>();
    if (debugWindow_->initialize()) {
        std::cout << "DebugWindow initialized successfully" << std::endl;
    }
#endif

    return true;
}

bool CompositionEngine::initialize(const DeviceConfig& config) {
    // Initialize internal RenderDevice
    if (!context_.initialize(config)) {
        return false;
    }

    ownsContext_ = true;
    device_ = context_.getDevice();
    format_ = context_.getSurfaceFormat();
    width_ = config.width;
    height_ = config.height;

    // Initialize input state
    inputState_.setResolution(width_, height_);

    // Initialize global filter chain
    globalFilterChain_.initialize(device_, format_, width_, height_);

    createIntermediateTextures();

    return true;
}

void CompositionEngine::createIntermediateTextures() {
    if (width_ == 0 || height_ == 0 || !device_) return;

    // Create intermediate texture for global post-processing
    wgpu::TextureDescriptor texDesc = {
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = {width_, height_, 1},
        .format = format_,
        .mipLevelCount = 1,
        .sampleCount = 1
    };

    globalIntermediateTexture_ = device_.CreateTexture(&texDesc);
    globalIntermediateView_ = globalIntermediateTexture_.CreateView();

    // Create intermediate textures for layers that have filters
    for (auto& entry : layers_) {
        if (!entry.filterChain.isEmpty()) {
            entry.needsIntermediateTexture = true;
            entry.intermediateTexture = device_.CreateTexture(&texDesc);
            entry.intermediateView = entry.intermediateTexture.CreateView();
        }
    }
}

size_t CompositionEngine::addLayer(std::unique_ptr<CompositionLayer> layer) {
    LayerEntry entry;
    entry.layer = std::move(layer);
    entry.filterChain.initialize(device_, format_, width_, height_);

    // Initialize the layer
    if (device_) {
        entry.layer->initialize(device_, format_);
    }

    layers_.push_back(std::move(entry));
    return layers_.size() - 1;
}

void CompositionEngine::removeLayer(size_t index) {
    if (index < layers_.size()) {
        layers_.erase(layers_.begin() + index);
    }
}

void CompositionEngine::clearLayers() {
    layers_.clear();
}

CompositionLayer* CompositionEngine::getLayer(size_t index) {
    if (index < layers_.size()) {
        return layers_[index].layer.get();
    }
    return nullptr;
}

FilterChain* CompositionEngine::getLayerFilterChain(size_t index) {
    if (index < layers_.size()) {
        return &layers_[index].filterChain;
    }
    return nullptr;
}

void CompositionEngine::sortLayersByOrder() {
    std::sort(layers_.begin(), layers_.end(),
        [](const LayerEntry& a, const LayerEntry& b) {
            return a.layer->getLayer() < b.layer->getLayer();
        });
}

void CompositionEngine::render(wgpu::TextureView outputView) {
    if (!device_) return;

    // Sort layers by z-order
    sortLayersByOrder();

    // Create command encoder
    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();

    // Determine final output target (with or without global filters)
    wgpu::TextureView finalTarget = outputView;
    if (!globalFilterChain_.isEmpty()) {
        finalTarget = globalIntermediateView_;
    }

    // Begin main render pass
    wgpu::RenderPassColorAttachment colorAttachment = {
        .view = finalTarget,
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = clearColor_
    };

    wgpu::RenderPassDescriptor renderPassDesc = {
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment
    };

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // Render each layer
    for (auto& entry : layers_) {
        if (!entry.layer->isEnabled()) continue;

        // If this layer has filters, we need to render to intermediate texture first
        if (!entry.filterChain.isEmpty()) {
            // TODO: Implement per-layer filter support
            // This requires rendering to intermediate texture, applying filters,
            // then compositing to the main render target
            // For now, just render directly
            entry.layer->render(pass);
        } else {
            // No filters, render directly
            entry.layer->render(pass);
        }
    }

    pass.End();

    // Apply global post-processing filters if any
    if (!globalFilterChain_.isEmpty()) {
        // Get the intermediate texture view (rendered scene without filters)
        // We need to create a texture view from the intermediate texture for input
        std::vector<wgpu::TextureView> inputs = {globalIntermediateView_};
        globalFilterChain_.apply(encoder, inputs, outputView, &inputState_);
    }

    // Submit commands
    wgpu::CommandBuffer commands = encoder.Finish();
    device_.GetQueue().Submit(1, &commands);
}

void CompositionEngine::update(float deltaTime) {
    // Update time information
    inputState_.time.delta = deltaTime;
    inputState_.time.elapsed += deltaTime;
    inputState_.time.frameCount++;

    // Update all layers
    for (auto& entry : layers_) {
        entry.layer->update(deltaTime);
    }

    // Reset per-frame input state at the end
    inputState_.resetPerFrameState();
}

void CompositionEngine::resize(uint32_t width, uint32_t height) {
    if (width_ == width && height_ == height) return;

    width_ = width;
    height_ = height;

    // Update input state resolution
    inputState_.setResolution(width_, height_);

    // Resize global filter chain
    globalFilterChain_.resize(width_, height_);

    // Recreate intermediate textures
    createIntermediateTextures();

    // Resize per-layer filter chains
    for (auto& entry : layers_) {
        entry.filterChain.resize(width_, height_);
    }
}

wgpu::Surface CompositionEngine::getSurface() const {
    if (ownsContext_) {
        return context_.getSurface();
    }
    return nullptr;
}

void CompositionEngine::present() {
    if (ownsContext_) {
        wgpu::Surface surface = context_.getSurface();
        if (surface) {
            surface.Present();
        }
    }
}

void CompositionEngine::render() {
    if (!ownsContext_) {
        // This method only works with internal RenderDevice
        // For external device usage, use update(deltaTime) + render(outputView) instead
        return;
    }

    // Calculate deltaTime automatically
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = 0.0f;

    if (!firstFrame_) {
        deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime_).count();
    } else {
        firstFrame_ = false;
    }

    lastFrameTime_ = currentTime;

    // Update all layers and animations
    update(deltaTime);

    wgpu::Surface surface = context_.getSurface();
    if (!surface) return;

    // Acquire surface texture
    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);

    if (!surfaceTexture.texture) return;

    // Create texture view
    wgpu::TextureView outputView = surfaceTexture.texture.CreateView();

    // Render all layers to the surface
    render(outputView);

    // Present the frame
    surface.Present();

#ifdef CLIPENGINE_DEBUG_WINDOW_ENABLED
    // Update debug window if enabled
    if (debugWindow_) {
        debugWindow_->update();
    }
#endif
}
