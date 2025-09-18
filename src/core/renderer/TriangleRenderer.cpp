#include "TriangleRenderer.h"

using namespace wgpu;

void TriangleRenderer::init()
{
    const float vertex_data[12] = {
        0.0f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f,
    };
    vertex_buffer_ = dawn::utils::CreateBufferFromData(device_, vertex_data, sizeof(vertex_data), BufferUsage::Vertex);

    module_ = dawn::utils::CreateShaderModule(device_, R"(
        @vertex fn vs(@location(0) pos : vec4f) -> @builtin(position) vec4f {
            return pos;
        }

        @fragment fn fs(@builtin(position) FragCoord : vec4f) -> @location(0) vec4f {
            return vec4f(1, 0, 0, 1);
        }
    )");

    dawn::utils::ComboRenderPipelineDescriptor desc;
    desc.layout = nullptr;
    desc.vertex.module = module_;
    desc.vertex.bufferCount = 1;
    desc.cBuffers[0].arrayStride = 4 * sizeof(float);
    desc.cBuffers[0].attributeCount = 1;
    desc.cAttributes[0].format = VertexFormat::Float32x4;
    desc.cFragment.module = module_;
    desc.cTargets[0].format = surface_texture_fmt_;

    pipeline_ = device_.CreateRenderPipeline(&desc);
}

void TriangleRenderer::render(wgpu::RenderPassEncoder& pass)
{
    pass.SetPipeline(pipeline_);
    pass.SetVertexBuffer(0, vertex_buffer_);
    pass.Draw(3);
}