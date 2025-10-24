#include "RuntimeInspector.h"
#include "CeLogger.h"

using namespace wgpu;

void RuntimeInspector::dumpSurfaceCaps(const Adapter& adapter, const Surface& surface)
{
    SurfaceCapabilities caps;
    surface.GetCapabilities(adapter, &caps);

    LOG_DEBUG("");
    LOG_DEBUG("Surface Capabilities");
    LOG_DEBUG("{:<1} {:<25} {}", "", "├─ Format count: ",       caps.formatCount);
    LOG_DEBUG("{:<1} {:<25} {}", "", "├─ Alpha mode count: ",   caps.alphaModeCount);
    LOG_DEBUG("{:<1} {:<25} {}", "", "└─ Present mode count: ", caps.presentModeCount);
    LOG_DEBUG("");

    if(caps.formatCount == 0) {
        LOG_ERROR("No supported surface formats available");
        return;
    }
}

void RuntimeInspector::dumpGPUInfo(const Adapter& adapter)
{
    AdapterInfo info;
    adapter.GetInfo(&info);

    LOG_DEBUG("");
    LOG_DEBUG("Adapter Information");
    LOG_DEBUG("{:<1} {:<25} {}", "", "├─ Vendor: ",       info.vendor.data);
    LOG_DEBUG("{:<1} {:<25} {}", "", "├─ Architecture: ", info.architecture.data);
    LOG_DEBUG("{:<1} {:<25} {}", "", "├─ Device: ",       info.device.data);
    LOG_DEBUG("{:<1} {:<25} {}", "", "├─ Description: ",  info.description.data);

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

    LOG_DEBUG("{:<1} {:<25} {}", "", "├─ Backend Type: ", backend);
    LOG_DEBUG("{:<1} {:<25} {}", "", "└─ Adapter Type: ", type);
    LOG_DEBUG("");
}

void RuntimeInspector::dumpNativeWindowInfo(const NativeWindow& native)
{
    LOG_DEBUG("{:<18} : {} x {}", " Window Size", native.width, native.height);
}
