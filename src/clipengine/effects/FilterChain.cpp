#include "FilterChain.h"
#include <iostream>

bool FilterChain::initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height) {
    device_ = device;
    format_ = format;
    width_ = width;
    height_ = height;

    createIntermediateTextures();

    // Initialize all existing filters
    for (auto& filter : filters_) {
        if (!filter->initialize(device_, format_)) {
            return false;
        }
    }

    return true;
}

void FilterChain::createIntermediateTextures() {
    if (width_ == 0 || height_ == 0 || !device_) return;

    // Create two intermediate textures for ping-pong rendering
    wgpu::TextureDescriptor texDesc = {
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = {width_, height_, 1},
        .format = format_,
        .mipLevelCount = 1,
        .sampleCount = 1
    };

    intermediateTexture0_ = device_.CreateTexture(&texDesc);
    intermediateTexture1_ = device_.CreateTexture(&texDesc);

    intermediateView0_ = intermediateTexture0_.CreateView();
    intermediateView1_ = intermediateTexture1_.CreateView();
}

void FilterChain::addFilter(std::unique_ptr<Filter> filter) {
    if (device_) {
        filter->initialize(device_, format_);
    }
    filters_.push_back(std::move(filter));
}

void FilterChain::removeFilter(size_t index) {
    if (index < filters_.size()) {
        filters_.erase(filters_.begin() + index);
    }
}

void FilterChain::clearFilters() {
    filters_.clear();
}

Filter* FilterChain::getFilter(size_t index) {
    if (index < filters_.size()) {
        return filters_[index].get();
    }
    return nullptr;
}

bool FilterChain::apply(wgpu::CommandEncoder& encoder,
                        const std::vector<wgpu::TextureView>& inputTextures,
                        wgpu::TextureView outputView,
                        const InputState* inputState) {
    if (filters_.empty()) {
        // No filters, just copy input to output
        // (In a real implementation, you might want to do a simple blit here)
        return true;
    }

    // Set input state for all filters and update parameters
    for (auto& filter : filters_) {
        if (filter->isEnabled()) {
            if (inputState) {
                filter->setInputState(inputState);
            }
            filter->updateParameters();
        }
    }

    // Track which view we're rendering to
    wgpu::TextureView currentInput = inputTextures.empty() ? nullptr : inputTextures[0];
    wgpu::TextureView currentOutput = nullptr;

    size_t enabledFilterCount = 0;
    for (const auto& filter : filters_) {
        if (filter->isEnabled()) {
            enabledFilterCount++;
        }
    }

    if (enabledFilterCount == 0) {
        return true;
    }

    size_t processedFilters = 0;
    bool useBuffer0 = true;

    for (size_t i = 0; i < filters_.size(); ++i) {
        auto& filter = filters_[i];
        if (!filter->isEnabled()) continue;

        processedFilters++;
        bool isLastFilter = (processedFilters == enabledFilterCount);

        // Determine output target
        if (isLastFilter) {
            currentOutput = outputView;
        } else {
            currentOutput = useBuffer0 ? intermediateView0_ : intermediateView1_;
        }

        // Create render pass
        wgpu::RenderPassColorAttachment colorAttachment = {
            .view = currentOutput,
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store,
            .clearValue = {0.0, 0.0, 0.0, 0.0}
        };

        wgpu::RenderPassDescriptor renderPassDesc = {
            .colorAttachmentCount = 1,
            .colorAttachments = &colorAttachment
        };

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

        // Apply filter
        if (processedFilters == 1) {
            // First filter uses the original input
            filter->apply(pass, inputTextures);
        } else {
            // Subsequent filters use the previous output
            filter->apply(pass, {currentInput});
        }

        pass.End();

        // For next iteration, current output becomes input
        if (!isLastFilter) {
            currentInput = currentOutput;
            useBuffer0 = !useBuffer0;
        }
    }

    return true;
}

void FilterChain::resize(uint32_t width, uint32_t height) {
    if (width_ == width && height_ == height) return;

    width_ = width;
    height_ = height;

    createIntermediateTextures();
}
