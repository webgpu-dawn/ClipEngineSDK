#include "WebGPUHelper.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace ClipEngine {

void WebGPUHelper::printAdapterInfo(wgpu::Adapter& adapter) {
    wgpu::AdapterInfo info;
    adapter.GetInfo(&info);

    std::cout << "WebGPU Adapter Information:" << std::endl;
    std::cout << "  Vendor: " << info.vendor.data << std::endl;
    std::cout << "  Architecture: " << info.architecture.data << std::endl;
    std::cout << "  Device: " << info.device.data << std::endl;
    std::cout << "  Driver: " << info.driver.data << std::endl;
    std::cout << "  Description: " << info.description.data << std::endl;

    std::cout << "  Backend Type: ";
    switch (info.backendType) {
        case wgpu::BackendType::D3D11: std::cout << "D3D11"; break;
        case wgpu::BackendType::D3D12: std::cout << "D3D12"; break;
        case wgpu::BackendType::Vulkan: std::cout << "Vulkan"; break;
        case wgpu::BackendType::OpenGL: std::cout << "OpenGL"; break;
        case wgpu::BackendType::OpenGLES: std::cout << "OpenGLES"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << std::endl;

    std::cout << "  Adapter Type: ";
    switch (info.adapterType) {
        case wgpu::AdapterType::DiscreteGPU: std::cout << "Discrete GPU"; break;
        case wgpu::AdapterType::IntegratedGPU: std::cout << "Integrated GPU"; break;
        case wgpu::AdapterType::CPU: std::cout << "CPU"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << std::endl;
}

wgpu::ShaderModule WebGPUHelper::createShaderFromFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    return createShader(code.c_str(), path);
}

}
