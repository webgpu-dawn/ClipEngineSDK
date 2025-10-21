#include "RuntimeInspector.h"

using namespace wgpu;

void RuntimeInspector::dumpGPUInfo(const Adapter& adapter) {
    AdapterInfo info;
    adapter.GetInfo(&info);


    LOG_INFO("==========================================");
    LOG_INFO("   WebGPU Adapter Information");
    LOG_INFO("==========================================");

    LOG_INFO("{:<18} {}", "Vendor:",       info.vendor.data);
    LOG_INFO("{:<18} {}", "Architecture:", info.architecture.data);
    LOG_INFO("{:<18} {}", "Device:",       info.device.data);
    LOG_INFO("{:<18} {}", "Description:",  info.description.data);

    std::string backend;
    switch (info.backendType) {
        case BackendType::D3D11:    backend = "D3D11"; break;
        case BackendType::D3D12:    backend = "D3D12"; break;
        case BackendType::Vulkan:   backend = "Vulkan"; break;
        case BackendType::OpenGL:   backend = "OpenGL"; break;
        case BackendType::OpenGLES: backend = "OpenGLES"; break;
        default: backend = "Unknown"; break;
    }

    std::string type;
    switch (info.adapterType) {
        case AdapterType::DiscreteGPU:   type = "Discrete GPU"; break;
        case AdapterType::IntegratedGPU: type = "Integrated GPU"; break;
        case AdapterType::CPU:           type = "CPU"; break;
        default: type = "Unknown"; break;
    }

    LOG_INFO("{:<18} {}", "Backend Type:", backend);
    LOG_INFO("{:<18} {}", "Adapter Type:", type);
    LOG_INFO("");
}

void RuntimeInspector::dumpNativeWindowInfo(const NativeWindow& native)
{
    LOG_INFO("{:<18} : {} x {}", " Window Size", native.width, native.height);
}
