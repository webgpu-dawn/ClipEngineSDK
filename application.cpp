#include "application.h"
#include "glfw3webgpu.h"

#include <iostream>

/**
 * 获取 adapter
 */
WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userData1, void* userData2) {
        UserData& userData = *reinterpret_cast<UserData*>(userData1);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            std::cout << "Could not get WebGPU adapter: " << message.data << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuInstanceRequestAdapter(
        instance,
        options,
        {
            .nextInChain = NULL,
            .mode = WGPUCallbackMode_AllowSpontaneous,   // WGPUCallbackMode_AllowSpontaneous 立即异步触发，不依赖额外时间循环
            .callback = onAdapterRequestEnded,
            .userdata1 = (void*)&userData,
            .userdata2 = NULL,
        }
    );

    return userData.adapter;
}

/**
 * 获取 device
 */
WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const* desc)
{
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    wgpuAdapterRequestDevice(
        adapter,
        desc,
        {
            .nextInChain = NULL,
            .mode = WGPUCallbackMode_AllowSpontaneous,
            .callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userData1, void* userData2) {
                UserData& userData = *reinterpret_cast<UserData*>(userData1);
                if (status == WGPURequestDeviceStatus_Success) {
                    userData.device = device;
                } else {
                    std::cout << "Could not get WebGPU device: " << message.data << std::endl;
                }
                userData.requestEnded = true;
            },
            .userdata1 = (void*)&userData,
            .userdata2 = NULL,
        }
    );

    return userData.device;
}

bool Application::initialize()
{
    if(!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
    if(!window_) {
        std::cerr << "Could not open window" << std::endl;
        glfwTerminate();
        return false;
    }

    // 创建 WebGPU 示例
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    instance_ = wgpuCreateInstance(&desc);

    // Check WebGPU instance
    if(!instance_) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }
    std::cout << "WebGPU instance : " << instance_ << std::endl;
    
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
    adapterOpts.compatibleSurface = glfwGetWGPUSurface(instance_, window_);

    WGPUAdapter adapter = requestAdapter(instance_, &adapterOpts);

    // 获取 adapter 限制能力
    WGPULimits limits = {};
    limits.nextInChain = nullptr;
    if(WGPUStatus_Success == wgpuAdapterGetLimits(adapter, &limits)) {
        std::cout << "Adapter Limits : " << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
    }

    // 枚举 WebGPU 实现支持的特性
    WGPUSupportedFeatures features = {};
    wgpuAdapterGetFeatures(adapter, &features);
    // std::cout << "Adapter Supported Features : " << std::endl;
    // for(uint32_t i = 0; i < features.featureCount; ++i) {
    //    std::cout << " - " << features.features[i] << std::endl;
    // }

    // 打印 adapter 信息
    WGPUAdapterInfo info = {};
    wgpuAdapterGetInfo(adapter, &info);
    std::cout << "Adapter Info : " << std::endl;
    std::cout << " - vendor: " << info.vendor.data << std::endl;
    std::cout << " - architecture: " << info.architecture.data << std::endl;
    std::cout << " - device: " << info.device.data << std::endl;
    std::cout << " - description: " << info.description.data << std::endl;
    std::cout << " - backendType:" << info.backendType << std::endl;
    std::cout << " - adapterType:" << info.adapterType << std::endl;

    // 获取 device
    WGPUDeviceDescriptor deviceDesc = {
        .nextInChain = nullptr,
        .label = "Device",
        .requiredFeatureCount = 0,
        .requiredLimits = nullptr,
        .defaultQueue = {
            .nextInChain = nullptr,
            .label = "Default Queue",
        },
        .deviceLostCallbackInfo = {
            .nextInChain = nullptr,
            .mode = WGPUCallbackMode_AllowSpontaneous,
            .callback = [](WGPUDevice const* device, WGPUDeviceLostReason reason, WGPUStringView message, void* userdata1, void* userdata2) {
                std::cout << "WebGPU Device Lost: " << message.data << std::endl;
            },
            .userdata1 = nullptr,
            .userdata2 = nullptr,
        }
    };
    device_ = requestDevice(adapter, &deviceDesc);
    // 获取 device 之后，建议立即释放 adapter
    wgpuAdapterRelease(adapter);
    std::cout << "Got Device : " << device_ << std::endl;

    // 获取 queue
    queue_ = wgpuDeviceGetQueue(device_);
    return true;
}

void Application::terminate()
{
    glfwDestroyWindow(window_);
    glfwTerminate();

    // 销毁 WebGPU 资源
    wgpuQueueRelease(queue_);
    wgpuDeviceRelease(device_);
    wgpuInstanceRelease(instance_);
}

void Application::run()
{
    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
    }
}