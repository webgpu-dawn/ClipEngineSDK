#include "ClipEngine.h"

using namespace std;
using namespace wgpu;

void ClipEngine::initialize(GLFWwindow* window)
{
    context_.initialize(window);

    tri_renderer_ = make_unique<TriangleRenderer>(context_.device_, context_.surface_texture_fmt_);
    tri_renderer_->init();

    tex_renderer_ = make_unique<TextureRenderer>(context_.device_, context_.surface_texture_fmt_);
    tex_renderer_->init();

}

void ClipEngine::render()
{
    wgpu::SurfaceTexture surface_texture;
    context_.surface_.GetCurrentTexture(&surface_texture);
    dawn::utils::ComboRenderPassDescriptor render_pass({ surface_texture.texture.CreateView()});

    wgpu::CommandEncoder encoder = context_.device_.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);  
        tex_renderer_->render(pass);
        // tri_renderer_->render(pass);   
        pass.End();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    context_.queue_.Submit(1, &commands);

    wgpu::Status status = context_.surface_.Present();
}

void ClipEngine::shutdown()
{

}