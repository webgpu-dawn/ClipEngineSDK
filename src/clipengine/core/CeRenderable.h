#pragma once
#include "../common/Common.h"
#include <string>

enum class CeRendererType {
    Video,
    Audio,
    Subtitle,
    Effect
};

struct CeRenderViewport {
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;
};

class CeRenderable {
public:
    virtual ~CeRenderable() = default;

    virtual bool initialize(wgpu::Device device, wgpu::TextureFormat format) = 0;
    virtual void render(wgpu::RenderPassEncoder& pass) = 0;
    virtual void update(float deltaTime) = 0;
    virtual CeRendererType getType() const = 0;
    virtual void setViewport(float x, float y, float width, float height) = 0;

    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    void setLayer(int layer) { layer_ = layer; }
    int getLayer() const { return layer_; }

    void setName(const std::string& name) { name_ = name; }
    void setName(const char* name) { name_ = name ? name : ""; }
    const std::string& getName() const { return name_; }

protected:
    wgpu::Device device_;
    wgpu::TextureFormat surfaceFormat_;
    bool enabled_ = true;
    int layer_ = 0;
    std::string name_;

    CeRenderViewport viewport_;
};

class CeRenderableFactory {
public:
    static std::unique_ptr<CeRenderable> createVideoRenderer();
};
