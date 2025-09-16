#include "ClipEngine.h"

using namespace std;
using namespace wgpu;

void ClipEngine::initialize(GLFWwindow* window)
{
    context_.initialize(window);

    static const float vertexData[12] = {
        0.0f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f,
    };
    vertex_buffer = dawn::utils::CreateBufferFromData(context_.device_, vertexData, sizeof(vertexData), BufferUsage::Vertex);
    wgpu::ShaderModule module = dawn::utils::CreateShaderModule(context_.device_, R"(
        @vertex fn vs(@location(0) pos : vec4f) -> @builtin(position) vec4f {
            return pos;
        }

        @fragment fn fs(@builtin(position) FragCoord : vec4f) -> @location(0) vec4f {
            return vec4f(1, 0, 0, 1);
        }
    )");

    dawn::utils::ComboRenderPipelineDescriptor desc;
    desc.layout = nullptr;
    desc.vertex.module = module;
    desc.vertex.bufferCount = 1;
    desc.cBuffers[0].arrayStride = 4 * sizeof(float);
    desc.cBuffers[0].attributeCount = 1;
    desc.cAttributes[0].format = wgpu::VertexFormat::Float32x4;
    desc.cFragment.module = module;
    desc.cTargets[0].format = context_.surface_texture_fmt_;

    context_.pipeline_ = context_.device_.CreateRenderPipeline(&desc);
}

void ClipEngine::render()
{
    wgpu::SurfaceTexture surface_texture;
    context_.surface_.GetCurrentTexture(&surface_texture);
    dawn::utils::ComboRenderPassDescriptor render_pass({ surface_texture.texture.CreateView()});

    wgpu::CommandEncoder encoder = context_.device_.CreateCommandEncoder();
    {
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
        pass.SetPipeline(context_.pipeline_);
        pass.SetVertexBuffer(0, vertex_buffer);
        pass.Draw(3);
        pass.End();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    context_.queue_.Submit(1, &commands);

    wgpu::Status status = context_.surface_.Present();
}

void ClipEngine::shutdown()
{

}