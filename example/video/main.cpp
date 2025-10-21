#include <iostream>
#include <chrono>
#include <thread>
#include "decoder.h"
#include <filesystem>

#include "clipengine/ClipEngine.h"
#include <clipengine/util/GPUTimer.h>

using namespace std;
namespace fs = std::filesystem;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}


void createWindow()
{
    

}

int main()
{
    const char CLASS_NAME[] = "clipforge";

    // 注册窗口类
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        CLASS_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hwnd) return -1;

    ShowWindow(hwnd, 1);

    fs::path exe_dir = fs::current_path();
    fs::current_path(exe_dir);

    CeEngineConfig config;
    config.width = 800;
    config.height = 600;
    config.title = "clipforge";

    CeEngine engine;
    if (!engine.initialize(config)) {
        std::cerr << "Failed to initialize render engine" << std::endl;
        return -1;
    }

    // 创建GPU计时器
    auto gpuTimer = std::make_shared<GPUTimer>(engine.getDevice());
    if (!gpuTimer->initialize()) {
        std::cerr << "Warning: GPU timestamp queries not supported, timing disabled" << std::endl;
    }

    // 设置GPU计时器到引擎
    engine.setGPUTimer(gpuTimer);

    auto videoRenderer = std::make_unique<VideoRenderer>();
    videoRenderer->setViewport(0.f, 0.0f, 1.0f, 1.0f);

    VideoRenderer* videoRendererPtr = videoRenderer.get();
    engine.addRenderer(std::move(videoRenderer));

    Decoder decoder;
    int frame_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    decoder.open_video("D:/video/8K.mp4", [&](AVFrame* frame) {
        if(frame->format == AV_PIX_FMT_D3D11) {
            ID3D11Texture2D* srcTex = (ID3D11Texture2D*)frame->data[0];
            int subIndex = (int)(intptr_t)frame->data[1];

            videoRendererPtr->updateFrame(srcTex, subIndex);
            engine.renderFrame();

            frame_count++;

            // 每 60 帧打印一次性能报告
            if (frame_count % 60 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration<double>(now - start_time).count();
                double fps = frame_count / elapsed;

                std::cout << "\n=== Performance Stats ===" << std::endl;
                std::cout << "Frames: " << frame_count << std::endl;
                std::cout << "FPS: " << fps << std::endl;

                if (gpuTimer->isSupported()) {
                    gpuTimer->printResults();
                    std::cout << "GPU Render Time: " << gpuTimer->getTime("Render") << " ms" << std::endl;
                }
            }
        }
    });

    // 最终性能报告
    std::cout << "\n=== Final Performance Report ===" << std::endl;
    if (gpuTimer->isSupported()) {
        gpuTimer->printResults();
    }

    while(!engine.shouldClose()) {
        engine.pollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    engine.shutdown();
    return 0;
}