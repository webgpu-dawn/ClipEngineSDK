#include "ClipEngine.h"
#include "CeContext.h"
#include "CeRenderable.h"
#include "../common/CeLogger.h"
#include <algorithm>

using namespace wgpu;

// ============================================================================
// pimpl_ Implementation Class
// ============================================================================

/**
 * @brief Private implementation of ClipEngine
 *
 * This class contains all the internal state and implementation details
 * of ClipEngine, hidden from the public API.
 */
class ClipEngine::Impl {
public:
    CeContext context_;
    std::vector<std::unique_ptr<CeRenderable>> renderers_;
    float backgroundColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    void sortRenderersByLayer() {
        std::sort(renderers_.begin(), renderers_.end(),
            [](const std::unique_ptr<CeRenderable>& a, const std::unique_ptr<CeRenderable>& b) {
                return a->getLayer() < b->getLayer();
            });
    }
};

ClipEngine::ClipEngine()
    : pimpl_(std::make_unique<Impl>()) {
    LOG_INFO("ClipEngine version : {}", "v1.0.0");
}

ClipEngine::~ClipEngine() {
    shutdown();
}

bool ClipEngine::initialize(const CeConfigure& config) {

    // 初始化 CeContext
    if (!pimpl_->context_.initialize(config)) {
        LOG_ERROR("Failed to initialize CeContext");
        return false;
    }

    LOG_INFO("ClipEngine initialized successfully");

    return true;
}

void ClipEngine::shutdown() {
    if (pimpl_) {
        pimpl_->renderers_.clear();
        pimpl_->context_.shutdown();
    }
}

// ============================================================================
// Renderer Management
// ============================================================================

void ClipEngine::addRenderer(std::unique_ptr<CeRenderable> renderer) {
    if (!renderer) return;

    if (!renderer->initialize(pimpl_->context_.getDevice(), pimpl_->context_.getSurfaceFormat())) {
        LOG_ERROR("Failed to initialize renderer");
        return;
    }

    pimpl_->renderers_.push_back(std::move(renderer));
    pimpl_->sortRenderersByLayer();
}

void ClipEngine::removeRenderer(CeRenderable* renderer) {
    auto it = std::remove_if(pimpl_->renderers_.begin(), pimpl_->renderers_.end(),
        [renderer](const std::unique_ptr<CeRenderable>& r) {
            return r.get() == renderer;
        });
    pimpl_->renderers_.erase(it, pimpl_->renderers_.end());
}

CeRenderable* ClipEngine::getRendererByName(const char* name) {
    if (!name) return nullptr;

    for (auto& renderer : pimpl_->renderers_) {
        if (renderer->getName() == name) {
            return renderer.get();
        }
    }
    return nullptr;
}

void ClipEngine::clear() {
    pimpl_->renderers_.clear();
}

// ============================================================================
// Rendering Control
// ============================================================================

void ClipEngine::renderFrame() {
    wgpu::Surface surface = pimpl_->context_.getSurface();
    wgpu::Device  device  = pimpl_->context_.getDevice();
    wgpu::Queue   queue   = pimpl_->context_.getQueue();

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

    wgpu::RenderPassColorAttachment colorAttachment = {
        .view = backbufferView,
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = {
            pimpl_->backgroundColor_[0],
            pimpl_->backgroundColor_[1],
            pimpl_->backgroundColor_[2],
            pimpl_->backgroundColor_[3]
        }
    };

    wgpu::RenderPassDescriptor renderPassDesc = {
        .colorAttachmentCount = 1,
        .colorAttachments     = &colorAttachment
    };

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

        for (auto& renderer : pimpl_->renderers_) {
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

void ClipEngine::update(float deltaTime) {
    for (auto& renderer : pimpl_->renderers_) {
        if (renderer->isEnabled()) {
            renderer->update(deltaTime);
        }
    }
}

// ============================================================================
// Window Management
// ============================================================================

// ============================================================================
// GPU Resource Access
// ============================================================================

wgpu::Device ClipEngine::getDevice() const {
    return pimpl_->context_.getDevice();
}

wgpu::Queue ClipEngine::getQueue() const {
    return pimpl_->context_.getQueue();
}

wgpu::TextureFormat ClipEngine::getSurfaceFormat() const {
    return pimpl_->context_.getSurfaceFormat();
}

CeContext& ClipEngine::getContext() {
    return pimpl_->context_;
}

const CeContext& ClipEngine::getContext() const {
    return pimpl_->context_;
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

std::vector<CeRenderable*> ClipEngine::getRenderersInternal() {
    std::vector<CeRenderable*> result;
    result.reserve(pimpl_->renderers_.size());
    for (auto& renderer : pimpl_->renderers_) {
        result.push_back(renderer.get());
    }
    return result;
}

const std::vector<CeRenderable*> ClipEngine::getRenderersInternal() const {
    std::vector<CeRenderable*> result;
    result.reserve(pimpl_->renderers_.size());
    for (auto& renderer : pimpl_->renderers_) {
        result.push_back(renderer.get());
    }
    return result;
}
