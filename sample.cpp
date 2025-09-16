#define GLFW_EXPOSE_NATIVE_WIN32
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>
#include <vector>

#include "ComboRenderPipelineDescriptor.h"
#include "WGPUHelpers.h"

using namespace std;
using namespace wgpu;

Instance instance_ = nullptr;
Adapter  adapter_  = nullptr;
Device   device_   = nullptr;
Queue    queue_    = nullptr;
Surface  surface_  = nullptr;
TextureFormat surface_texture_fmt = TextureFormat::BGRA8Unorm;
RenderPipeline pipeline_;


int width = 640;
int height= 480;

GLFWwindow* window = nullptr;

std::unique_ptr<wgpu::ChainedStruct> SetupWindowAndGetSurfaceDescriptor(GLFWwindow* window) {
    // 创建平台扩展结构体
    auto desc = std::make_unique<wgpu::SurfaceDescriptorFromWindowsHWND>();
    desc->hwnd = glfwGetWin32Window(window);
    desc->hinstance = GetModuleHandle(nullptr);
    return desc;
}

Surface CreateSurfaceForWindow(const Instance& instance, GLFWwindow* window)
{
    auto chainedDescriptor = SetupWindowAndGetSurfaceDescriptor(window);

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = chainedDescriptor.get();
    wgpu::Surface surface = instance.CreateSurface(&descriptor);

    return surface;
}

int main() {
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
            [](RequestAdapterStatus status, Adapter adapter, StringView message){
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
    DeviceDescriptor device_desc = {};
    device_desc.nextInChain = nullptr;
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
            [](RequestDeviceStatus status, Device device, StringView message){
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

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(width, height, "Learn WebGPU", nullptr, nullptr);

    // 4、创建、配置 surface
    surface_ = CreateSurfaceForWindow(instance_, window);
    SurfaceCapabilities capabilities;
    surface_.GetCapabilities(adapter_, &capabilities);
    SurfaceConfiguration config = {};
    config.device = device_;
    config.format = capabilities.formats[0];
    config.width  = width;
    config.height = height;
    config.presentMode = capabilities.presentModes[0];
    surface_.Configure(&config);
    surface_texture_fmt = capabilities.formats[0];
    
    static const float vertexData[12] = {
        0.0f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f,
    };
    Buffer vertex_buffer = dawn::utils::CreateBufferFromData(device_, vertexData, sizeof(vertexData), BufferUsage::Vertex);
    wgpu::ShaderModule module = dawn::utils::CreateShaderModule(device_, R"(
        @vertex fn vs(@location(0) pos : vec4f) -> @builtin(position) vec4f {
            return pos;
        }

        @fragment fn fs(@builtin(position) FragCoord : vec4f) -> @location(0) vec4f {
            return vec4f(1, 0, 0, 1);
        }
    )");

    dawn::utils::ComboRenderPipelineDescriptor desc;
    desc.layout = nullptr;
    desc.vertex.module = module;
    desc.vertex.bufferCount = 1;
    desc.cBuffers[0].arrayStride = 4 * sizeof(float);
    desc.cBuffers[0].attributeCount = 1;
    desc.cAttributes[0].format = wgpu::VertexFormat::Float32x4;
    desc.cFragment.module = module;
    desc.cTargets[0].format = surface_texture_fmt;

    pipeline_ = device_.CreateRenderPipeline(&desc);

    while(!glfwWindowShouldClose(window)) {
        wgpu::SurfaceTexture surface_texture;
        surface_.GetCurrentTexture(&surface_texture);
        dawn::utils::ComboRenderPassDescriptor render_pass({ surface_texture.texture.CreateView()});
    
        wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
            pass.SetPipeline(pipeline_);
            pass.SetVertexBuffer(0, vertex_buffer);
            pass.Draw(3);
            pass.End();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue_.Submit(1, &commands);

        wgpu::Status status = surface_.Present();
        glfwPollEvents();
    }
    return 1;
}
