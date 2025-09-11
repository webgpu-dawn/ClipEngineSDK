#include <webgpu/webgpu.h>
#include <iostream>
#include <cassert>
#include <vector>

/**
 * 获取 adapter
 */
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
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

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.nextInChain = NULL;
    // WGPUCallbackMode_AllowSpontaneous 立即异步触发，不依赖额外时间循环
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    callbackInfo.callback = onAdapterRequestEnded;
    callbackInfo.userdata1 = (void*)&userData;
    callbackInfo.userdata2 = NULL;
    
    wgpuInstanceRequestAdapter(instance, options, callbackInfo);

    return userData.adapter;
}

int main(int argc, char** argv)
{
    // Create WebGPU instance
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    WGPUInstance instance = wgpuCreateInstance(&desc);

    // Check WebGPU instance
    if(!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }
    std::cout << "WebGPU instance : " << instance << std::endl;
    
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;

    WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
    std::cout << "Got Adapter : " << adapter << std::endl;

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

    WGPUAdapterInfo info = {};
    wgpuAdapterGetInfo(adapter, &info);
    std::cout << "Adapter Info : " << std::endl;
    std::cout << " - vendor: " << info.vendor.data << std::endl;
    std::cout << " - architecture: " << info.architecture.data << std::endl;
    std::cout << " - device: " << info.device.data << std::endl;
    std::cout << " - description: " << info.description.data << std::endl;
    std::cout << " - backendType:" << info.backendType << std::endl;
    std::cout << " - adapterType:" << info.adapterType << std::endl;

    while (true) {
        _sleep(1000);
    }

    // Destroy WebGPU instance
    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);
    return 0;
}