#include "ClipEngine.h"

#include <iostream>

using namespace std;
using namespace wgpu;

void ClipEngine::initialize(GLFWwindow* window)
{
    context_.initialize(window);

    std::unique_ptr<TriangleRenderer> tri_renderer = make_unique<TriangleRenderer>(context_.device_, context_.surface_texture_fmt_);
    tri_renderer->init();

    std::unique_ptr<TextureRenderer> tex_renderer = make_unique<TextureRenderer>(context_.device_, context_.surface_texture_fmt_);
    tex_renderer->init();

    renderers_.push_back(std::move(tex_renderer));
    // renderers_.push_back(std::move(tri_renderer));

}

void ClipEngine::render()
{
    wgpu::SurfaceTexture surface_texture;
    context_.surface_.GetCurrentTexture(&surface_texture);
    dawn::utils::ComboRenderPassDescriptor render_pass({ surface_texture.texture.CreateView()});

    wgpu::CommandEncoder encoder = context_.device_.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass); 
        std::cout << "rendering ..." << std::endl;
        for(int i = 0; i < renderers_.size(); i++) {
            renderers_[i]->render(pass);
        }   
        pass.End();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    context_.queue_.Submit(1, &commands);

    wgpu::Status status = context_.surface_.Present();
}

void ClipEngine::shutdown()
{

}