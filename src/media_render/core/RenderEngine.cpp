#include "RenderEngine.h"

namespace MediaRender {

bool RenderEngine::initialize(wgpu::Device device, wgpu::TextureFormat format) {
    device_ = device;
    surfaceFormat_ = format;
    return true;
}

void RenderEngine::addRenderer(std::unique_ptr<IMediaRenderer> renderer) {
    if(!renderer) return;

    if(!renderer->initialize(device_, surfaceFormat_)) {
        return;
    }

    renderers_.push_back(std::move(renderer));
    sortRenderersByLayer();
}

void RenderEngine::removeRenderer(IMediaRenderer* renderer) {
    renderers_.erase(
        std::remove_if(renderers_.begin(), renderers_.end(),
            [renderer](const std::unique_ptr<IMediaRenderer>& r) {
                return r.get() == renderer;
            }),
        renderers_.end()
    );
}

void RenderEngine::render(wgpu::RenderPassEncoder& pass) {
    for(auto& renderer : renderers_) {
        if(renderer->isEnabled()) {
            renderer->render(pass);
        }
    }
}

void RenderEngine::update(float deltaTime) {
    for(auto& renderer : renderers_) {
        if(renderer->isEnabled()) {
            renderer->update(deltaTime);
        }
    }
}

IMediaRenderer* RenderEngine::getRendererByType(RendererType type) {
    for(auto& renderer : renderers_) {
        if(renderer->getType() == type) {
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

} // namespace MediaRender
