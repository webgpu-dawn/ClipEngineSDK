#include "RuntimeInspector.h"
#include "CeLogger.h"

using namespace wgpu;

void RuntimeInspector::dumpGPUInfo(const Adapter& adapter) {
    AdapterInfo info;
    adapter.GetInfo(&info);


    LOG_DEBUG("==========================================");
    LOG_DEBUG("   WebGPU Adapter Information");
    LOG_DEBUG("==========================================");

    LOG_DEBUG("{:<18} {}", "Vendor:",       info.vendor.data);
    LOG_DEBUG("{:<18} {}", "Architecture:", info.architecture.data);
    LOG_DEBUG("{:<18} {}", "Device:",       info.device.data);
    LOG_DEBUG("{:<18} {}", "Description:",  info.description.data);

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

    LOG_DEBUG("{:<18} {}", "Backend Type:", backend);
    LOG_DEBUG("{:<18} {}", "Adapter Type:", type);
    LOG_DEBUG("");
}

void RuntimeInspector::dumpNativeWindowInfo(const NativeWindow& native)
{
    LOG_DEBUG("{:<18} : {} x {}", " Window Size", native.width, native.height);
}
