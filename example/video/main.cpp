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
        }
    });

    while(!engine.shouldClose()) {
        engine.pollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    engine.shutdown();
    return 0;
}