// 简单的媒体播放器示例
// 演示如何使用 MediaRender 引擎渲染视频和音频波形

#include "../core/RenderEngine.h"
#include "../core/MediaRenderer.h"
#include "../renderers/VideoRenderer.h"
#include "../renderers/AudioWaveformRenderer.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <webgpu/webgpu_cpp.h>
#include <iostream>

using namespace MediaRender;

class SimplePlayer {
public:
    bool initialize() {
        // 1. 初始化 GLFW
        if(!glfwInit()) return false;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window_ = glfwCreateWindow(1280, 720, "Media Render Engine - Simple Player", nullptr, nullptr);
        if(!window_) return false;

        // 2. 创建 WebGPU 实例
        wgpu::InstanceDescriptor instDesc = {};
        instance_ = wgpu::CreateInstance(&instDesc);

        // 3. 创建 Surface
        surface_ = createSurface();

        // 4. 请求 Adapter
        wgpu::RequestAdapterOptions adapterOpts = {};
        adapterOpts.compatibleSurface = surface_;

        wgpu::Adapter adapter;
        instance_.RequestAdapter(&adapterOpts, wgpu::CallbackMode::AllowSpontaneous,
            [&](wgpu::RequestAdapterStatus status, wgpu::Adapter result, char const* message) {
                if(status == wgpu::RequestAdapterStatus::Success) {
                    adapter = result;
                }
            });

        if(!adapter) return false;

        // 5. 请求 Device
        wgpu::DeviceDescriptor deviceDesc = {};
        static constexpr wgpu::FeatureName features[] = {
            wgpu::FeatureName::SharedTextureMemoryDXGISharedHandle,
            wgpu::FeatureName::DawnMultiPlanarFormats
        };
        deviceDesc.requiredFeatureCount = 2;
        deviceDesc.requiredFeatures = features;

        adapter.RequestDevice(&deviceDesc, wgpu::CallbackMode::AllowSpontaneous,
            [&](wgpu::RequestDeviceStatus status, wgpu::Device result, char const* message) {
                if(status == wgpu::RequestDeviceStatus::Success) {
                    device_ = result;
                }
            });

        if(!device_) return false;

        // 6. 配置 Surface
        wgpu::SurfaceCapabilities capabilities;
        surface_.GetCapabilities(adapter, &capabilities);

        wgpu::SurfaceConfiguration config = {};
        config.device = device_;
        config.format = capabilities.formats[0];
        config.width = 1280;
        config.height = 720;
        config.presentMode = wgpu::PresentMode::Fifo;
        surface_.Configure(&config);

        surfaceFormat_ = capabilities.formats[0];

        // 7. 初始化渲染引擎
        renderEngine_.initialize(device_, surfaceFormat_);
        renderEngine_.setBackgroundColor(0.1f, 0.1f, 0.1f, 1.0f);

        // 8. 创建视频渲染器
        auto videoRenderer = RendererFactory::createVideoRenderer();
        videoRenderer->setViewport(0.0f, 0.0f, 1.0f, 0.8f);  // 上部 80%
        videoRenderer->setFillMode(VideoRenderer::FillMode::Fit);
        videoRenderer->setLayer(0);
        renderEngine_.addRenderer(std::move(videoRenderer));

        // 9. 创建音频波形渲染器
        auto audioRenderer = RendererFactory::createAudioWaveformRenderer();
        audioRenderer->setViewport(0.0f, 0.8f, 1.0f, 0.2f);  // 下部 20%
        audioRenderer->setWaveformColor(0.0f, 1.0f, 0.0f, 0.8f);
        audioRenderer->setLayer(1);
        renderEngine_.addRenderer(std::move(audioRenderer));

        return true;
    }

    void run() {
        lastTime_ = glfwGetTime();

        while(!glfwWindowShouldClose(window_)) {
            glfwPollEvents();

            // 计算 delta time
            double currentTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentTime - lastTime_);
            lastTime_ = currentTime;

            // 更新
            renderEngine_.update(deltaTime);

            // 渲染
            render();
        }
    }

    void render() {
        // 获取当前 surface texture
        wgpu::SurfaceTexture surfaceTexture;
        surface_.GetCurrentTexture(&surfaceTexture);

        if(surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
            return;
        }

        wgpu::TextureView view = surfaceTexture.texture.CreateView();

        // 创建 render pass
        wgpu::RenderPassColorAttachment attachment = {};
        attachment.view = view;
        attachment.loadOp = wgpu::LoadOp::Clear;
        attachment.storeOp = wgpu::StoreOp::Store;
        attachment.clearValue = {0.1f, 0.1f, 0.1f, 1.0f};

        wgpu::RenderPassDescriptor passDesc = {};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &attachment;

        // 编码渲染命令
        wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);

        // 渲染所有启用的渲染器
        renderEngine_.render(pass);

        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        device_.GetQueue().Submit(1, &commands);

        // Present
        surface_.Present();
    }

    void cleanup() {
        if(window_) {
            glfwDestroyWindow(window_);
        }
        glfwTerminate();
    }

    // 更新视频帧的示例接口
    void updateVideoFrame(ID3D11Texture2D* texture, int arrayIndex) {
        auto* videoRenderer = renderEngine_.getRenderer<VideoRenderer>();
        if(videoRenderer) {
            videoRenderer->updateFrame(texture, arrayIndex);
        }
    }

    // 更新音频数据的示例接口
    void updateAudioData(const float* samples, size_t count, int channels) {
        auto* audioRenderer = renderEngine_.getRenderer<AudioWaveformRenderer>();
        if(audioRenderer) {
            audioRenderer->updateAudioData(samples, count, channels);
        }
    }

private:
    wgpu::Surface createSurface() {
#ifdef _WIN32
        wgpu::SurfaceDescriptorFromWindowsHWND desc = {};
        desc.hwnd = glfwGetWin32Window(window_);
        desc.hinstance = GetModuleHandle(nullptr);

        wgpu::SurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain = &desc;

        return instance_.CreateSurface(&surfaceDesc);
#else
        return nullptr;
#endif
    }

    GLFWwindow* window_ = nullptr;
    wgpu::Instance instance_;
    wgpu::Surface surface_;
    wgpu::Device device_;
    wgpu::TextureFormat surfaceFormat_;

    RenderEngine renderEngine_;

    double lastTime_ = 0.0;
};

int main() {
    SimplePlayer player;

    if(!player.initialize()) {
        std::cerr << "Failed to initialize player" << std::endl;
        return -1;
    }

    std::cout << "Media Render Engine - Simple Player" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    player.run();
    player.cleanup();

    return 0;
}
