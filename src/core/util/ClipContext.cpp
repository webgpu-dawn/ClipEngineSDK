#include "ClipContext.h"

#include <iostream>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

using namespace std;
using namespace wgpu;

std::unique_ptr<wgpu::ChainedStruct> ClipContext::setup_window_and_get_surface_desciptor(GLFWwindow* window)
{
    // 创建平台扩展结构体
    auto desc = std::make_unique<wgpu::SurfaceDescriptorFromWindowsHWND>();

    // HWND hwnd = FindWindow(nullptr, "clipforge");
    // desc->hwnd = hwnd;
    desc->hwnd = glfwGetWin32Window(window);
    desc->hinstance = GetModuleHandle(nullptr);
    return desc;
}

Surface ClipContext::create_surface_for_window(const Instance& instance, GLFWwindow* window)
{
    auto chainedDescriptor = setup_window_and_get_surface_desciptor(window);

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = chainedDescriptor.get();
    wgpu::Surface surface = instance.CreateSurface(&descriptor);

    return surface;
}

bool ClipContext::initialize(GLFWwindow* window)
{
    // 1、创建 instance
    InstanceDescriptor instance_desc = {};
    instance_desc.nextInChain = nullptr;
    static constexpr auto timed_wait_any = InstanceFeatureName::TimedWaitAny;
    instance_desc.requiredFeatureCount = 1;
    instance_desc.requiredFeatures = &timed_wait_any;
    instance_ = CreateInstance(&instance_desc);

    // 2、创建 adapter
    RequestAdapterOptions adapter_opts = {};
    adapter_opts.nextInChain = nullptr;
    adapter_opts.backendType = BackendType::Undefined;
    adapter_opts.powerPreference = PowerPreference::HighPerformance;

    instance_.WaitAny(
        instance_.RequestAdapter(
            &adapter_opts, CallbackMode::WaitAnyOnly,
            [=](RequestAdapterStatus status, Adapter adapter, StringView message){
                if(status != RequestAdapterStatus::Success) {
                    cerr << "Failed to get an adapter : " << message.data << endl;
                    return;
                }
                adapter_ = std::move(adapter);
            }
        ),UINT64_MAX
    );

    if(adapter_ == nullptr) {
        return -1;
    }

    AdapterInfo info;
    adapter_.GetInfo(&info);
    cout << "Adapter info : " << endl;
    cout << "   vendor : " << info.vendor.data << endl;
    cout << "   architecture : " << info.architecture.data << endl; 
    cout << "   device : " << info.device.data << endl;
    cout << "   subgroupSizes : { min : " << info.subgroupMinSize << " max : " << info.subgroupMaxSize << " }" << endl;

    // 3、创建 device
    Limits limits;
    limits.maxTextureDimension2D = 16384;
    limits.maxBufferSize = 2147483648;
    DeviceDescriptor device_desc = {};
    device_desc.nextInChain = nullptr;
    device_desc.requiredLimits = &limits;
    device_desc.SetDeviceLostCallback(
        CallbackMode::AllowSpontaneous,
        [](const Device&, DeviceLostReason reason, StringView message){
            const char* reason_name = "";
            switch(reason) {
                case DeviceLostReason::Unknown:
                    reason_name = "Unknown";
                    break;
                case DeviceLostReason::Destroyed:
                    reason_name = "Destroyed";
                    break;
                case DeviceLostReason::CallbackCancelled:
                    reason_name = "CallbackCanceled";
                    break;
                case DeviceLostReason::FailedCreation:
                    reason_name = "FailedCreation";
                    break;
                default:
                    break;
            }
            cerr << "Device lost because of " << reason_name << endl;
        }
    );
    device_desc.SetUncapturedErrorCallback(
        [](const Device&, ErrorType type, StringView message){
            const char* type_name = "";
            switch(type) {
                case ErrorType::Validation:
                    type_name = "Validation";
                    break;
                case ErrorType::OutOfMemory:
                    type_name = "OutOfMemory";
                    break;
                case ErrorType::Internal:
                    type_name = "Internal";
                    break;
                case ErrorType::Unknown:
                    type_name = "Unknown";
                    break;
                default:
                    break;
            }
            cerr << type_name  << " error: " << message.data << endl;
        }
    );
    instance_.WaitAny(
        adapter_.RequestDevice(
            &device_desc, CallbackMode::WaitAnyOnly,
            [=](RequestDeviceStatus status, Device device, StringView message){
                if(status != RequestDeviceStatus::Success) {
                    cerr << "Failed to get an device : " << message.data << endl;
                    return;
                }
                device_ = std::move(device);
                queue_  = device_.GetQueue();
            }
        ),
        UINT64_MAX
    );
    if(device_ == nullptr) {
        return -1;
    }

    // 4、创建、配置 surface
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    surface_ = create_surface_for_window(instance_, window);
    SurfaceCapabilities capabilities;
    surface_.GetCapabilities(adapter_, &capabilities);
    SurfaceConfiguration config = {};
    config.device = device_;
    config.format = capabilities.formats[0];
    config.width  = width;
    config.height = height;
    config.presentMode = capabilities.presentModes[0];
    surface_.Configure(&config);
    surface_texture_fmt_ = capabilities.formats[0];

    return true;
}

void ClipContext::begin()
{

}

void ClipContext::end()
{

}