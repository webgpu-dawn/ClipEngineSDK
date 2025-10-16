#include <iostream>
#include <chrono>
#include <thread>
#include "decoder.h"
#include <filesystem>

#include <clipengine/ClipEngine.h>

using namespace std;
namespace fs = std::filesystem;

int main()
{
    fs::path exe_dir = fs::current_path();
    fs::current_path(exe_dir);

    ClipEngine::RenderEngineConfig config;
    config.width = 800;
    config.height = 600;
    config.title = "Video Player - ClipEngine";

    ClipEngine::RenderEngine engine;
    if (!engine.initialize(config)) {
        std::cerr << "Failed to initialize render engine" << std::endl;
        return -1;
    }

    auto videoRenderer = std::make_unique<ClipEngine::VideoRenderer>();
    videoRenderer->setViewport(0.f, 0.0f, 1.0f, 1.0f);

    ClipEngine::VideoRenderer* videoRendererPtr = videoRenderer.get();
    engine.addRenderer(std::move(videoRenderer));

    Decoder decoder;
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