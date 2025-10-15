#pragma once
#include "core/renderer/IRenderable.h"
#include <webgpu/webgpu_cpp.h>

#include <dawn/native/D3D12Backend.h>

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

    void update_texture(ID3D11Texture2D* tex, int index);

private:
    void init_buffer();
    void init_sampler();
    void init_texture();
    void init_shader();
    void init_bindgroup();
    void init_pipeline();

    void init_8k_texture();

    void init_compute_pipeline_and_bind_group();

private:
    // 顶点缓冲 & 采样器
    wgpu::Buffer vertex_buffer_;
    wgpu::Sampler sampler_;

    wgpu::Buffer uniform_buffer_;

    // 渲染相关
    

    wgpu::TextureView y_tex_;
    wgpu::TextureView u_tex_;
    wgpu::TextureView v_tex_;

    wgpu::TextureView rgba_tex_;

    // ----------------------------
    // Compute Shader 相关
    // ----------------------------
    wgpu::ComputePipeline compute_pipeline_;
    wgpu::BindGroup computeBindGroup_;

    
};
