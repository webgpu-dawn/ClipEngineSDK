#pragma once
#include "core/renderer/IRenderable.h"
#include <webgpu/webgpu_cpp.h>

#include <dawn/native/D3D12Backend.h>
#include <dawn/native/D3D11Backend.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

class FrameRenderer : public IRenderable
{
public:
    FrameRenderer(wgpu::Device device, wgpu::TextureFormat format)
        : IRenderable(device, format) { }

    void init();
    void render(wgpu::RenderPassEncoder& pass);

    // 更新纹理：从硬解码 D3D11 NV12 纹理导入到 Dawn
    void update_texture(ID3D11Texture2D* tex, int index);

private:
    void init_buffer();
    void init_sampler();
    void init_shader();
    void init_bindgroup();
    void init_pipeline();

private:
    // 渲染资源
    wgpu::Buffer vertex_buffer_;
    wgpu::Sampler sampler_;

    // NV12 双平面纹理视图
    wgpu::TextureView y_tex_;   // Y 平面 (R8Unorm)
    wgpu::TextureView uv_tex_;  // UV 平面 (RG8Unorm)
};
