#pragma once
#include "../common/Common.h"
#include "Renderer.h"

#include <dawn/webgpu_cpp_print.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

#include "../core/CeContext.h"

// Forward declaration to avoid exposing GLFW in public header
struct GLFWwindow;

namespace ClipEngine {

class GPUTimer;

struct RenderEngineConfig {
    uint32_t width = 800;
    uint32_t height = 600;
    const char* title = "ClipEngine";
    bool vsync = true;
    wgpu::PresentMode presentMode = wgpu::PresentMode::Fifo;
    CeContextMode mode = CeContextMode::FIND_WINDOW;
};

class RenderEngine {
public:
    RenderEngine() = default;
    ~RenderEngine();

    bool initialize(const RenderEngineConfig& config);
    void shutdown();

    void addRenderer(std::unique_ptr<IMediaRenderer> renderer);
    void removeRenderer(IMediaRenderer* renderer);

    void renderFrame();
    void update(float deltaTime);

    bool shouldClose() const;
    void pollEvents();

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

    wgpu::Device getDevice() const { return context_.getDevice(); }
    wgpu::Queue getQueue() const { return context_.getQueue(); }
    wgpu::TextureFormat getSurfaceFormat() const { return context_.getSurfaceFormat(); }
    GLFWwindow* getWindow() const { return context_.getWindow(); }

    CeContext& getContext() { return context_; }

    // GPU Timer
    void setGPUTimer(std::shared_ptr<GPUTimer> timer);
    std::shared_ptr<GPUTimer> getGPUTimer() const;

private:
    void sortRenderersByLayer();

    CeContext context_;

    std::vector<std::unique_ptr<IMediaRenderer>> renderers_;
    float backgroundColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    std::shared_ptr<GPUTimer> gpuTimer_;
};

} // namespace ClipEngine
