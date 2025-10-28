#include "OffscreenRenderer.h"
#include "../utils/CeLogger.h"

OffscreenRenderer::~OffscreenRenderer() {
    // WebGPU objects are reference-counted, will auto-release
}

bool OffscreenRenderer::initialize(wgpu::Device device, wgpu::TextureFormat format, uint32_t width, uint32_t height) {
    if (!device || width == 0 || height == 0) {
        LOG_ERROR("Invalid parameters for OffscreenRenderer");
        return false;
    }

    device_ = device;
    format_ = format;
    width_ = width;
    height_ = height;

    createRenderTarget();
    createStagingBuffer();

    LOG_INFO("OffscreenRenderer initialized: {}x{}, format={}", width_, height_, static_cast<int>(format_));
    return true;
}

void OffscreenRenderer::createRenderTarget() {
    // Create offscreen render target texture
    wgpu::TextureDescriptor texDesc = {
        .usage = wgpu::TextureUsage::RenderAttachment |
                 wgpu::TextureUsage::CopySrc,  // CopySrc needed for readback
        .dimension = wgpu::TextureDimension::e2D,
        .size = {width_, height_, 1},
        .format = format_,
        .mipLevelCount = 1,
        .sampleCount = 1
    };

    renderTargetTexture_ = device_.CreateTexture(&texDesc);
    renderTargetView_ = renderTargetTexture_.CreateView();

    if (!renderTargetTexture_ || !renderTargetView_) {
        LOG_ERROR("Failed to create offscreen render target");
    }
}

void OffscreenRenderer::createStagingBuffer() {
    // Create staging buffer for GPU→CPU readback
    // Buffer must be aligned to COPY_BUFFER_ALIGNMENT (256 bytes)
    size_t bytesPerRow = width_ * getBytesPerPixel();
    size_t alignedBytesPerRow = (bytesPerRow + 255) & ~255;  // Align to 256
    size_t bufferSize = alignedBytesPerRow * height_;

    wgpu::BufferDescriptor bufDesc = {
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .size = bufferSize,
        .mappedAtCreation = false
    };

    stagingBuffer_ = device_.CreateBuffer(&bufDesc);

    if (!stagingBuffer_) {
        LOG_ERROR("Failed to create staging buffer for pixel readback");
    }
}

uint32_t OffscreenRenderer::getBytesPerPixel() const {
    switch (format_) {
        case wgpu::TextureFormat::RGBA8Unorm:
        case wgpu::TextureFormat::BGRA8Unorm:
        case wgpu::TextureFormat::RGBA8UnormSrgb:
        case wgpu::TextureFormat::BGRA8UnormSrgb:
            return 4;
        case wgpu::TextureFormat::RGB10A2Unorm:
            return 4;
        default:
            LOG_WARN("Unknown texture format bytes per pixel, assuming 4");
            return 4;
    }
}

void OffscreenRenderer::readPixels(std::function<void(const uint8_t*, size_t)> callback) {
    if (!callback) {
        LOG_ERROR("readPixels called with null callback");
        return;
    }

    pendingCallback_ = callback;

    // Create command encoder for copy operation
    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();

    // Setup copy parameters with proper alignment
    size_t bytesPerRow = width_ * getBytesPerPixel();
    size_t alignedBytesPerRow = (bytesPerRow + 255) & ~255;  // Align to 256

    wgpu::TexelCopyTextureInfo source;
    source.texture = renderTargetTexture_;
    source.mipLevel = 0;
    source.origin = {0, 0, 0};
    source.aspect = wgpu::TextureAspect::All;

    wgpu::TexelCopyBufferInfo destination;
    destination.buffer = stagingBuffer_;
    destination.layout.offset = 0;
    destination.layout.bytesPerRow = static_cast<uint32_t>(alignedBytesPerRow);
    destination.layout.rowsPerImage = height_;

    wgpu::Extent3D copySize = {width_, height_, 1};

    // Copy texture to staging buffer
    encoder.CopyTextureToBuffer(&source, &destination, &copySize);

    // Submit commands
    wgpu::CommandBuffer commands = encoder.Finish();
    device_.GetQueue().Submit(1, &commands);

    // Map buffer for reading (async)
    auto mapCallback = [this](wgpu::MapAsyncStatus status, wgpu::StringView message) {
        if (status != wgpu::MapAsyncStatus::Success) {
            std::string msgStr(message.data, message.length);
            LOG_ERROR("Buffer mapping failed: {}", msgStr);
            return;
        }

        if (pendingCallback_) {
            const uint8_t* data = static_cast<const uint8_t*>(
                stagingBuffer_.GetConstMappedRange(0, getBufferSize())
            );

            if (data) {
                pendingCallback_(data, getBufferSize());
            } else {
                LOG_ERROR("Failed to get mapped range");
            }

            stagingBuffer_.Unmap();
            pendingCallback_ = nullptr;
        }
    };

    stagingBuffer_.MapAsync(
        wgpu::MapMode::Read,
        0,
        alignedBytesPerRow * height_,
        wgpu::CallbackMode::AllowSpontaneous,
        mapCallback
    );
}

bool OffscreenRenderer::readPixelsSync(uint8_t* buffer, size_t bufferSize) {
    if (!buffer) {
        LOG_ERROR("readPixelsSync called with null buffer");
        return false;
    }

    size_t requiredSize = getBufferSize();
    if (bufferSize < requiredSize) {
        LOG_ERROR("Buffer too small: provided {}, required {}", bufferSize, requiredSize);
        return false;
    }

    bool dataReady = false;
    const uint8_t* mappedData = nullptr;

    // Use readPixels with a lambda that captures our local variables
    readPixels([&](const uint8_t* data, size_t size) {
        mappedData = data;
        dataReady = true;
    });

    // Wait for the operation to complete
    waitForCompletion();

    if (dataReady && mappedData) {
        // Handle aligned bytes per row
        size_t bytesPerRow = width_ * getBytesPerPixel();
        size_t alignedBytesPerRow = (bytesPerRow + 255) & ~255;

        // Copy row by row, skipping padding
        for (uint32_t row = 0; row < height_; ++row) {
            memcpy(
                buffer + row * bytesPerRow,
                mappedData + row * alignedBytesPerRow,
                bytesPerRow
            );
        }

        return true;
    }

    return false;
}

void OffscreenRenderer::waitForCompletion() {
    // Process callbacks until mapping is complete
    // In Dawn, this is typically done via device.Tick() or queue.Submit()
    // For blocking wait, we poll the device

    #ifdef __EMSCRIPTEN__
        // Emscripten doesn't support blocking waits
        LOG_WARN("waitForCompletion() not supported in Emscripten, use async readPixels()");
    #else
        // Poll device until buffer is mapped
        int maxIterations = 1000;  // Timeout after ~1 second
        while (pendingCallback_ && maxIterations-- > 0) {
            device_.Tick();
            #ifdef _WIN32
                Sleep(1);  // 1ms sleep
            #else
                usleep(1000);  // 1ms sleep
            #endif
        }

        if (maxIterations <= 0) {
            LOG_ERROR("waitForCompletion() timed out");
        }
    #endif
}
