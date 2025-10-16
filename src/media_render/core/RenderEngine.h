#pragma once
#include "MediaRenderer.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace MediaRender {

class RenderEngine {
public:
    RenderEngine() = default;
    ~RenderEngine() = default;

    bool initialize(wgpu::Device device, wgpu::TextureFormat format);

    void addRenderer(std::unique_ptr<IMediaRenderer> renderer);

    void removeRenderer(IMediaRenderer* renderer);

    void render(wgpu::RenderPassEncoder& pass);

    void update(float deltaTime);

    template<typename T>
    T* getRenderer() {
        for(auto& renderer : renderers_) {
            if(auto* typed = dynamic_cast<T*>(renderer.get())) {
                return typed;
            }
        }
        return nullptr;
    }

    template<typename T>
    std::vector<T*> getRenderers() {
        std::vector<T*> result;
        for(auto& renderer : renderers_) {
            if(auto* typed = dynamic_cast<T*>(renderer.get())) {
                result.push_back(typed);
            }
        }
        return result;
    }

    IMediaRenderer* getRendererByType(RendererType type);

    void clear();

    void setBackgroundColor(float r, float g, float b, float a = 1.0f);

private:
    void sortRenderersByLayer();

    wgpu::Device device_;
    wgpu::TextureFormat surfaceFormat_;
    std::vector<std::unique_ptr<IMediaRenderer>> renderers_;

    float backgroundColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

} // namespace MediaRender
