#pragma once
#include "Renderer.h"
#include <webgpu/webgpu_cpp.h>
#include <dawn/webgpu_cpp_print.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

// Forward declaration to avoid exposing GLFW in public header
struct GLFWwindow;

namespace ClipEngine {

struct RenderEngineConfig {
    uint32_t width = 800;
    uint32_t height = 600;
    const char* title = "ClipEngine";
    bool vsync = true;
    wgpu::PresentMode presentMode = wgpu::PresentMode::Fifo;
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

    wgpu::Device getDevice() const { return device_; }
    wgpu::Queue getQueue() const { return queue_; }
    wgpu::TextureFormat getSurfaceFormat() const { return surfaceFormat_; }
    GLFWwindow* getWindow() const { return window_; }

private:
    bool initializeWindow(const RenderEngineConfig& config);
    bool initializeWebGPU();
    void sortRenderersByLayer();

    GLFWwindow* window_ = nullptr;
    wgpu::Instance instance_;
    wgpu::Adapter adapter_;
    wgpu::Device device_;
    wgpu::Queue queue_;
    wgpu::Surface surface_;
    wgpu::TextureFormat surfaceFormat_ = wgpu::TextureFormat::BGRA8Unorm;

    std::vector<std::unique_ptr<IMediaRenderer>> renderers_;
    float backgroundColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    uint32_t width_ = 800;
    uint32_t height_ = 600;
};

} // namespace ClipEngine
