#include "CeEngine.h"
#include "../util/GPUTimer.h"

using namespace wgpu;

CeEngine::~CeEngine() {
    shutdown();
}

bool CeEngine::initialize(const CeEngineConfig& config) {
    // 配置 CeContext
    CeContextConfig contextConfig;
    contextConfig.mode = config.mode;
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
    queue.Submit(1, &commands);

    // Read GPU timing results after submission
    if (gpuTimer_ && gpuTimer_->isSupported()) {
        gpuTimer_->readResults();
    }

    surface.Present();
}

void CeEngine::update(float deltaTime) {
    for (auto& renderer : renderers_) {
        if (renderer->isEnabled()) {
            renderer->update(deltaTime);
        }
    }
}

bool CeEngine::shouldClose() const {
    GLFWwindow* window = context_.getWindow();
    // 只有在有 GLFW 窗口时才检查
    return window && glfwWindowShouldClose(window);
}

void CeEngine::pollEvents() {
    // 只有在有 GLFW 窗口时才调用 GLFW 的事件循环
    GLFWwindow* window = context_.getWindow();
    if (window) {
        glfwPollEvents();
    }
    // FIND_WINDOW 模式下，窗口事件由外部窗口的消息循环处理
}

CeRenderable* CeEngine::getRendererByType(CeRendererType type) {
    for (auto& renderer : renderers_) {
        if (renderer->getType() == type) {
            return renderer.get();
        }
    }
    return nullptr;
}

void CeEngine::clear() {
    renderers_.clear();
}

void CeEngine::setBackgroundColor(float r, float g, float b, float a) {
    backgroundColor_[0] = r;
    backgroundColor_[1] = g;
    backgroundColor_[2] = b;
    backgroundColor_[3] = a;
}

void CeEngine::sortRenderersByLayer() {
    std::sort(renderers_.begin(), renderers_.end(),
        [](const std::unique_ptr<CeRenderable>& a, const std::unique_ptr<CeRenderable>& b) {
            return a->getLayer() < b->getLayer();
        });
}

void CeEngine::setGPUTimer(std::shared_ptr<GPUTimer> timer) {
    gpuTimer_ = timer;
}

std::shared_ptr<GPUTimer> CeEngine::getGPUTimer() const {
    return gpuTimer_;
}
