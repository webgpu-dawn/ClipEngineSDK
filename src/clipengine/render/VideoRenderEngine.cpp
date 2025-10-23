#include "VideoRenderEngine.h"
#include "TextureRenderer.h"
#include <iostream>

bool VideoRenderEngine::initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height) {
    device_ = device;
    format_ = format;
    width_ = width;
    height_ = height;

    // Initialize input state
    inputState_.setResolution(width_, height_);

    // Initialize global filter chain
    globalFilterChain_.initialize(device_, format_, width_, height_);

    createIntermediateTextures();

    return true;
}

void VideoRenderEngine::createIntermediateTextures() {
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

    // Create intermediate textures for renderables that have filters
    for (auto& entry : renderables_) {
        if (!entry.filterChain.isEmpty()) {
            entry.needsIntermediateTexture = true;
            entry.intermediateTexture = device_.CreateTexture(&texDesc);
            entry.intermediateView = entry.intermediateTexture.CreateView();
        }
    }
}

size_t VideoRenderEngine::addRenderable(std::unique_ptr<CeRenderable> renderable) {
    RenderableEntry entry;
    entry.renderable = std::move(renderable);
    entry.filterChain.initialize(device_, format_, width_, height_);

    // Initialize the renderable
    if (device_) {
        entry.renderable->initialize(device_, format_);
    }

    renderables_.push_back(std::move(entry));
    return renderables_.size() - 1;
}

void VideoRenderEngine::removeRenderable(size_t index) {
    if (index < renderables_.size()) {
        renderables_.erase(renderables_.begin() + index);
    }
}

void VideoRenderEngine::clearRenderables() {
    renderables_.clear();
}

CeRenderable* VideoRenderEngine::getRenderable(size_t index) {
    if (index < renderables_.size()) {
        return renderables_[index].renderable.get();
    }
    return nullptr;
}

FilterChain* VideoRenderEngine::getRenderableFilterChain(size_t index) {
    if (index < renderables_.size()) {
        return &renderables_[index].filterChain;
    }
    return nullptr;
}

void VideoRenderEngine::sortRenderablesByLayer() {
    std::sort(renderables_.begin(), renderables_.end(),
        [](const RenderableEntry& a, const RenderableEntry& b) {
            return a.renderable->getLayer() < b.renderable->getLayer();
        });
}

void VideoRenderEngine::render(wgpu::TextureView outputView) {
    if (!device_) return;

    // Sort by layer
    sortRenderablesByLayer();

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

    // Render each object
    for (auto& entry : renderables_) {
        if (!entry.renderable->isEnabled()) continue;

        // If this renderable has filters, we need to render to intermediate texture first
        if (!entry.filterChain.isEmpty()) {
            // TODO: Implement per-object filter support
            // This requires rendering to intermediate texture, applying filters,
            // then compositing to the main render target
            // For now, just render directly
            entry.renderable->render(pass);
        } else {
            // No filters, render directly
            entry.renderable->render(pass);
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

void VideoRenderEngine::update(float deltaTime) {
    // Update time information
    inputState_.time.delta = deltaTime;
    inputState_.time.elapsed += deltaTime;
    inputState_.time.frameCount++;

    // Update renderables
    for (auto& entry : renderables_) {
        entry.renderable->update(deltaTime);
    }

    // Reset per-frame input state at the end
    inputState_.resetPerFrameState();
}

void VideoRenderEngine::resize(uint32_t width, uint32_t height) {
    if (width_ == width && height_ == height) return;

    width_ = width;
    height_ = height;

    // Update input state resolution
    inputState_.setResolution(width_, height_);

    // Resize global filter chain
    globalFilterChain_.resize(width_, height_);

    // Recreate intermediate textures
    createIntermediateTextures();

    // Resize per-renderable filter chains
    for (auto& entry : renderables_) {
        entry.filterChain.resize(width_, height_);
    }
}
