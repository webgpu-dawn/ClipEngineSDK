#include "RenderEngine.h"
#include "../util/GPUTimer.h"
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <dawn/native/DawnNative.h>
#include <iostream>

using namespace wgpu;

namespace ClipEngine {

RenderEngine::~RenderEngine() {
    shutdown();
}

bool RenderEngine::initialize(const RenderEngineConfig& config) {
    width_ = config.width;
    height_ = config.height;

    if (!initializeWindow(config)) {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }

    if (!initializeWebGPU()) {
        std::cerr << "Failed to initialize WebGPU" << std::endl;
        return false;
    }

    std::cout << "ClipEngine initialized successfully" << std::endl;
    std::cout << "  Resolution: " << width_ << "x" << height_ << std::endl;
    std::cout << "  Surface Format: " << surfaceFormat_ << std::endl;

    return true;
}

bool RenderEngine::initializeWindow(const RenderEngineConfig& config) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window_ = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    return true;
}

bool RenderEngine::initializeWebGPU() {
    constexpr InstanceFeatureName requiredFeatures[] ={
        InstanceFeatureName::TimedWaitAny
    };
    wgpu::InstanceDescriptor instanceDesc = {
        .requiredFeatureCount = std::size(requiredFeatures),
        .requiredFeatures = requiredFeatures
    };
    instance_ = wgpu::CreateInstance(&instanceDesc);
    if (!instance_) {
        std::cerr << "Failed to create WebGPU instance" << std::endl;
        return false;
    }

    surface_ = wgpu::Surface::Acquire(glfwGetWGPUSurface(instance_.Get(), window_));
    if (!surface_) {
        std::cerr << "Failed to create surface" << std::endl;
        return false;
    }

    wgpu::RequestAdapterOptions adapterOpts = {};
    adapterOpts.compatibleSurface = surface_;
    adapterOpts.powerPreference = wgpu::PowerPreference::HighPerformance;

    instance_.WaitAny(
        instance_.RequestAdapter(
            &adapterOpts, wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
                if (status != wgpu::RequestAdapterStatus::Success) {
                    std::cerr << "Failed to get adapter: " << message.data << std::endl;
                    return;
                }
                adapter_ = std::move(adapter);
            }
        ), UINT64_MAX
    );

    if (!adapter_) {
        std::cerr << "Failed to acquire adapter" << std::endl;
        return false;
    }

    wgpu::AdapterInfo adapterInfo;
    adapter_.GetInfo(&adapterInfo);
    std::cout << "WebGPU Adapter:" << std::endl;
    std::cout << "  Vendor: " << adapterInfo.vendor.data << std::endl;
    std::cout << "  Device: " << adapterInfo.device.data << std::endl;
    std::cout << "  Architecture: " << adapterInfo.architecture.data << std::endl;

    wgpu::FeatureName deviceFeatures[] = {
        wgpu::FeatureName::SharedTextureMemoryDXGISharedHandle,
        wgpu::FeatureName::DawnMultiPlanarFormats,
        wgpu::FeatureName::TimestampQuery
    };

    // Enable unsafe APIs toggle for timestamp queries
    const char* enabledToggles[] = {"allow_unsafe_apis"};
    wgpu::DawnTogglesDescriptor togglesDesc = {};
    togglesDesc.enabledToggleCount = 1;
    togglesDesc.enabledToggles = enabledToggles;

    wgpu::DeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = &togglesDesc;
    deviceDesc.requiredFeatureCount = 3;
    deviceDesc.requiredFeatures = deviceFeatures;
    deviceDesc.defaultQueue.label = "ClipEngine Queue";
    deviceDesc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
            std::cerr << "WebGPU Uncaptured Error: " << message.data << std::endl;
        }
    );

    instance_.WaitAny(
        adapter_.RequestDevice(
            &deviceDesc, wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
                if (status != wgpu::RequestDeviceStatus::Success) {
                    std::cerr << "Failed to get device: " << message.data << std::endl;
                    return;
                }
                device_ = std::move(device);
                queue_ = device_.GetQueue();
            }
        ), UINT64_MAX
    );

    if (!device_) {
        std::cerr << "Failed to acquire device" << std::endl;
        return false;
    }

    wgpu::SurfaceCapabilities surfaceCaps;
    surface_.GetCapabilities(adapter_, &surfaceCaps);

    surfaceFormat_ = surfaceCaps.formats[0];
    for (uint32_t i = 0; i < surfaceCaps.formatCount; ++i) {
        if (surfaceCaps.formats[i] == wgpu::TextureFormat::BGRA8Unorm) {
            surfaceFormat_ = wgpu::TextureFormat::BGRA8Unorm;
            break;
        }
    }

    wgpu::SurfaceConfiguration surfaceConfig = {};
    surfaceConfig.device = device_;
    surfaceConfig.format = surfaceFormat_;
    surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfig.width = width_;
    surfaceConfig.height = height_;
    surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
    surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Opaque;

    surface_.Configure(&surfaceConfig);

    return true;
}

void RenderEngine::shutdown() {
    renderers_.clear();

    if (surface_) {
        surface_.Unconfigure();
        surface_ = nullptr;
    }

    queue_ = nullptr;
    device_ = nullptr;
    adapter_ = nullptr;
    instance_ = nullptr;

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
}

void RenderEngine::addRenderer(std::unique_ptr<IMediaRenderer> renderer) {
    if (!renderer) return;

    if (!renderer->initialize(device_, surfaceFormat_)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return;
    }

    renderers_.push_back(std::move(renderer));
    sortRenderersByLayer();
}

void RenderEngine::removeRenderer(IMediaRenderer* renderer) {
    auto it = std::remove_if(renderers_.begin(), renderers_.end(),
        [renderer](const std::unique_ptr<IMediaRenderer>& r) {
            return r.get() == renderer;
        });
    renderers_.erase(it, renderers_.end());
}

void RenderEngine::renderFrame() {
    wgpu::SurfaceTexture surfaceTexture;
    surface_.GetCurrentTexture(&surfaceTexture);

    if (!surfaceTexture.texture) {
        std::cerr << "Failed to get current surface texture" << std::endl;
        return;
    }

    wgpu::TextureView backbufferView = surfaceTexture.texture.CreateView();

    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = backbufferView;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {
        backgroundColor_[0],
        backgroundColor_[1],
        backgroundColor_[2],
        backgroundColor_[3]
    };

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();

    // GPU timing: begin render timestamp
    if (gpuTimer_ && gpuTimer_->isSupported()) {
        gpuTimer_->beginQuery(encoder, "Render");
    }

    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

        for (auto& renderer : renderers_) {
            if (renderer->isEnabled()) {
                renderer->render(pass);
            }
        }

        pass.End();
    }

    // GPU timing: end render timestamp
    if (gpuTimer_ && gpuTimer_->isSupported()) {
        gpuTimer_->endQuery(encoder);
        gpuTimer_->resolveQueries(encoder);
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue_.Submit(1, &commands);

    // Read GPU timing results after submission
    if (gpuTimer_ && gpuTimer_->isSupported()) {
        gpuTimer_->readResults();
    }

    surface_.Present();
}

void RenderEngine::update(float deltaTime) {
    for (auto& renderer : renderers_) {
        if (renderer->isEnabled()) {
            renderer->update(deltaTime);
        }
    }
}

bool RenderEngine::shouldClose() const {
    return window_ && glfwWindowShouldClose(window_);
}

void RenderEngine::pollEvents() {
    glfwPollEvents();
}

IMediaRenderer* RenderEngine::getRendererByType(RendererType type) {
    for (auto& renderer : renderers_) {
        if (renderer->getType() == type) {
            return renderer.get();
        }
    }
    return nullptr;
}

void RenderEngine::clear() {
    renderers_.clear();
}

void RenderEngine::setBackgroundColor(float r, float g, float b, float a) {
    backgroundColor_[0] = r;
    backgroundColor_[1] = g;
    backgroundColor_[2] = b;
    backgroundColor_[3] = a;
}

void RenderEngine::sortRenderersByLayer() {
    std::sort(renderers_.begin(), renderers_.end(),
        [](const std::unique_ptr<IMediaRenderer>& a, const std::unique_ptr<IMediaRenderer>& b) {
            return a->getLayer() < b->getLayer();
        });
}

void RenderEngine::setGPUTimer(std::shared_ptr<GPUTimer> timer) {
    gpuTimer_ = timer;
}

std::shared_ptr<GPUTimer> RenderEngine::getGPUTimer() const {
    return gpuTimer_;
}

} // namespace ClipEngine
