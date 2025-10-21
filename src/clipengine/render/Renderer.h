#pragma once
#include <webgpu/webgpu_cpp.h>
#include <memory>
#include <vector>

enum class RendererType {
    Video,
    Audio,
    Subtitle,
    Effect
};

class IMediaRenderer {
public:
    virtual ~IMediaRenderer() = default;

    virtual bool initialize(wgpu::Device device, wgpu::TextureFormat format) = 0;
    virtual void render(wgpu::RenderPassEncoder& pass) = 0;
    virtual void update(float deltaTime) = 0;
    virtual RendererType getType() const = 0;
    virtual void setViewport(float x, float y, float width, float height) = 0;

    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    void setLayer(int layer) { layer_ = layer; }
    int getLayer() const { return layer_; }

protected:
    wgpu::Device device_;
    wgpu::TextureFormat surfaceFormat_;
    bool enabled_ = true;
    int layer_ = 0;

    struct Viewport {
        float x = 0.0f, y = 0.0f;
        float width = 1.0f, height = 1.0f;
    } viewport_;
};

class RendererFactory {
public:
    static std::unique_ptr<IMediaRenderer> createVideoRenderer();
};