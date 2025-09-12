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
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

    // 创建 WebGPU 示例
    WGPUInstance instance = wgpuCreateInstance(nullptr);
    std::cout << "Request adapter ... " << std::endl;

    surface_ = glfwGetWGPUSurface(instance, window_);
    
    // 获取 adapter
    WGPURequestAdapterOptions adapterOpts = {
        .nextInChain = nullptr,
        .powerPreference = WGPUPowerPreference_HighPerformance,
        .compatibleSurface = surface_
    };
    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
    std::cout << "Got adapter : " << adapter << std::endl;
    wgpuInstanceRelease(instance);

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

    std::cout << "Request device ... " << std::endl;
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
                std::cout << "WebGPU Device lost: " << message.data << std::endl;
            },
            .userdata1 = nullptr,
            .userdata2 = nullptr,
        }
    };
    device_ = requestDevice(adapter, &deviceDesc);
    std::cout << "Got device : " << device_ << std::endl;

    // 获取 queue
    queue_ = wgpuDeviceGetQueue(device_);

    WGPUSurfaceCapabilities cap = {};
    WGPUStatus status = wgpuSurfaceGetCapabilities(surface_, adapter, &cap);
    std::cout << "Supported formats = " << cap.formatCount << std::endl;

    WGPUTextureFormat format = cap.formats[0];
    // 配置 surface
    WGPUSurfaceConfiguration config = {
        .nextInChain = nullptr,
        .device = device_,
        .format = cap.formats[0],
        .usage = WGPUTextureUsage_RenderAttachment,
        .width = 640,
        .height = 480,
        .viewFormatCount = 0,
        .viewFormats = nullptr,
        .alphaMode = WGPUCompositeAlphaMode_Auto,
        .presentMode = WGPUPresentMode_Fifo
    };
    
    wgpuSurfaceConfigure(surface_, &config);
    //  Release the adapter only after it has been fully utilized
    wgpuAdapterRelease(adapter);
    return true;
}

void Application::terminate()
{
    // 销毁 WebGPU 资源
    wgpuSurfaceUnconfigure(surface_);
    wgpuQueueRelease(queue_);
    wgpuSurfaceRelease(surface_);
    wgpuDeviceRelease(device_);
    glfwDestroyWindow(window_);
    glfwTerminate();
}

WGPUTextureView Application::get_next_surface_textureview()
{
    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(surface_, &surface_texture);

    if(surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
        std::cerr << "Could not get next surface texture: " << surface_texture.status << std::endl;
        return nullptr;
    }

    // Create a view for this surface texuture
    WGPUTextureViewDescriptor desc = {
        .nextInChain = nullptr,
        .label = "Surface Texture View",
        .format = wgpuTextureGetFormat(surface_texture.texture),
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_All,
    };
    WGPUTextureView view = wgpuTextureCreateView(surface_texture.texture, &desc);

    wgpuTextureRelease(surface_texture.texture);

    return view;
}

void Application::run()
{
    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        // Get the next target texture view
        WGPUTextureView view = get_next_surface_textureview();
        if(!view) return;

        // Create a command encoder for the draw call
        WGPUCommandEncoderDescriptor encoder_desc = {
            .nextInChain = nullptr,
            .label = "command encoder"
        };
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device_, &encoder_desc);

        // Create the render pass that clears the screen with our color
        WGPURenderPassColorAttachment render_pass_color_attachment = {
            .view = view,
            .resolveTarget = nullptr,
            .loadOp = WGPULoadOp_Clear,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = WGPUColor { 1.0, 0, 0, 1.0 },
        #ifdef WEBGPU_BACKEND_WGPU
            .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED
        #endif
        };

        WGPURenderPassDescriptor render_pass_desc = {
            .nextInChain = nullptr,
            .colorAttachmentCount = 1,
            .colorAttachments = &render_pass_color_attachment,
            .depthStencilAttachment = nullptr,
            .timestampWrites = nullptr
        };

        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);

        WGPUCommandBufferDescriptor cmd_buffer_desc = {
            .nextInChain = nullptr,
            .label = "Command buffer"
        };
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        wgpuCommandEncoderRelease(encoder);

        std::cout << "Submitting command ..." << std::endl;
        wgpuQueueSubmit(queue_, 1, &command);
        wgpuCommandBufferRelease(command);
        std::cout << "Command submitted ." << std::endl;

        wgpuSurfacePresent(surface_);
        
        wgpuDeviceTick(device_);

        wgpuTextureViewRelease(view);
    }
}