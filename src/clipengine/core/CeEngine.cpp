#include "CeEngine.h"
#include "../util/GPUTimer.h"

using namespace wgpu;

CeEngine::~CeEngine() {
    shutdown();
}

bool CeEngine::initialize(const CeEngineConfig& config) {
    // 配置 CeContext
    CeContextConfig contextConfig;
    contextConfig.windowTitle = config.title;
    contextConfig.width = config.width;
    contextConfig.height = config.height;

    // 初始化 CeContext
    if (!context_.initialize(contextConfig)) {
        LOG_ERROR("Failed to initialize CeContext");
        return false;
    }

    LOG_INFO("CeEngine initialized successfully");

    return true;
}

void CeEngine::shutdown() {
    renderers_.clear();
    context_.shutdown();
}

void CeEngine::addRenderer(std::unique_ptr<CeRenderable> renderer) {
    if (!renderer) return;

    if (!renderer->initialize(context_.getDevice(), context_.getSurfaceFormat())) {
        LOG_ERROR("Failed to initialize renderer");
        return;
    }

    renderers_.push_back(std::move(renderer));
    sortRenderersByLayer();
}

void CeEngine::removeRenderer(CeRenderable* renderer) {
    auto it = std::remove_if(renderers_.begin(), renderers_.end(),
        [renderer](const std::unique_ptr<CeRenderable>& r) {
            return r.get() == renderer;
        });
    renderers_.erase(it, renderers_.end());
}

void CeEngine::renderFrame() {
    wgpu::Surface surface = context_.getSurface();
    wgpu::Device device = context_.getDevice();
    wgpu::Queue queue = context_.getQueue();

    if (!surface) {
        LOG_ERROR("Surface is null");
        return;
    }

    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);

    if (!surfaceTexture.texture) {
        LOG_ERROR("Failed to get current surface texture");
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

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    // GPU timing: begin render timestamp
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

        for (auto& renderer : renderers_) {
            if (renderer->isEnabled()) {
                renderer->render(pass);
            }
        }

        pass.End();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    surface.Present();
}

void CeEngine::update(float deltaTime) {
    for (auto& renderer : renderers_) {
        if (renderer->isEnabled()) {
            renderer->update(deltaTime);
        }
    }
}

bool CeEngine::shouldClose()
{
    return false;
}

void CeEngine::pollEvents() {
    
}


void CeEngine::clear() {
    renderers_.clear();
}

void CeEngine::sortRenderersByLayer() {
    std::sort(renderers_.begin(), renderers_.end(),
        [](const std::unique_ptr<CeRenderable>& a, const std::unique_ptr<CeRenderable>& b) {
            return a->getLayer() < b->getLayer();
        });
}
