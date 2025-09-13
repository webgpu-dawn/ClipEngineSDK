#include "application.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <glfw3webgpu.h>

using namespace wgpu;
using namespace std;

const char* shader_source = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
	if (in_vertex_index == 0u) {
		return vec4f(-0.45, 0.5, 0.0, 1.0);
	} else if (in_vertex_index == 1u) {
		return vec4f(0.45, 0.5, 0.0, 1.0);
	} else {
		return vec4f(0.0, -0.5, 0.0, 1.0);
	}
}
// Add this in the same shaderSource literal than the vertex entry point
@fragment
fn fs_main() -> @location(0) vec4f {
	return vec4f(0.0, 0.4, 0.7, 1.0);
}
)";

bool Application::initialize_pipeline()
{
    // ShaderModule
    ShaderSourceWGSL wgsl_desc = Default;
    wgsl_desc.code = StringView(shader_source);
    ShaderModuleDescriptor shader_desc = Default;
    shader_desc.nextInChain = &wgsl_desc.chain;
    shader_desc.label = StringView("Shader source");
    ShaderModule shader_module = device_.createShaderModule(shader_desc);

    // VertexState
    VertexState vertex_state = Default;
    vertex_state.module = shader_module;
    vertex_state.entryPoint = StringView("vs_main");

    // FragmentState
    // ColorTargetState
    // BlendState
    BlendState blend_state = Default;
    ColorTargetState color_target = Default;
    color_target.format = surface_format_;
    color_target.blend = &blend_state;

    FragmentState fragment_state = Default;
    fragment_state.module = shader_module;
    fragment_state.entryPoint = StringView("fs_main");
    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;


    RenderPipelineDescriptor pipeline_desc = Default;
    pipeline_desc.vertex = vertex_state;
    pipeline_desc.fragment = &fragment_state;
    pipeline_ = device_.createRenderPipeline(pipeline_desc);

    shader_module.release();
    return true;
}

bool Application::initialize()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

    // 1、创建 WebGPU 示例
    instance_ = createInstance();

    surface_ = glfwCreateWindowWGPUSurface(instance_, window_);

    // 2、获取 adapter
    std::cout << "Request adapter ... " << std::endl;
    RequestAdapterOptions adapter_opts = Default;
    adapter_opts.powerPreference = WGPUPowerPreference_HighPerformance;
    adapter_opts.compatibleSurface = surface_;
    Adapter adapter = requestAdapterSync(instance_, &adapter_opts);
    std::cout << "Got adapter: " << adapter << std::endl;

    // 3、获取 device
    std::cout << "Request device ..." << std::endl;
    std::vector<FeatureName> features;
    Limits required_limits = Default;
    DeviceDescriptor device_desc = Default;
    device_desc.label = StringView("device");
    device_desc.requiredFeatureCount = features.size();
    device_desc.requiredFeatures = (WGPUFeatureName*)features.data();
    device_desc.requiredLimits = &required_limits;
    device_desc.defaultQueue.label = StringView("queue");
    device_desc.deviceLostCallbackInfo.callback = [](
        WGPUDevice const* device,
        WGPUDeviceLostReason reason,
        struct WGPUStringView message,
        void* /* userdata1 */,
        void* /* userdata2 */
        ) {
            std::cout
                << "Device " << device << " was lost: reason " << reason
                << " (" << StringView(message) << ")" // NEW
                << std::endl;
        };
    device_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    device_desc.uncapturedErrorCallbackInfo.callback = []( // TODO: setter
        WGPUDevice const* device,
        WGPUErrorType type,
        struct WGPUStringView message,
        void* /* userdata1 */,
        void* /* userdata2 */
        ) {
            std::cout
                << "Uncaptured error in device " << device << ": type " << type
                << " (" << StringView(message) << ")" // NEW
                << std::endl;
        };
    device_ = requestDeviceSync(instance_, adapter, &device_desc);
    std::cout << "Got device: " << device_ << std::endl;

    // 4、获取 queue
    queue_ = device_.getQueue();

    // 5、配置 surface
    SurfaceCapabilities capabilities = Default;
    Status status = surface_.getCapabilities(adapter, &capabilities);
    if (status != Status::Success) return false;

    SurfaceConfiguration config = Default;
    config.width = 640;
    config.height = 480;
    config.device = device_;
    config.format = capabilities.formats[0];
    config.presentMode = PresentMode::Fifo;
    config.alphaMode = CompositeAlphaMode::Auto;
    surface_.configure(config);
    surface_format_ = config.format;

    capabilities.freeMembers();
    adapter.release();

    if (!initialize_pipeline()) return false;

    return true;
}

void Application::terminate()
{
    // 销毁 WebGPU 资源
    surface_.unconfigure();
    queue_.release();
    surface_.release();
    device_.release();

    glfwDestroyWindow(window_);
    glfwTerminate();
}

wgpu::TextureView Application::get_next_surface_view()
{
    SurfaceTexture surface_texture = Default;
    surface_.getCurrentTexture(&surface_texture);
    if (
        surface_texture.status != SurfaceGetCurrentTextureStatus::SuccessOptimal &&
        surface_texture.status != SurfaceGetCurrentTextureStatus::SuccessSuboptimal
        ) {
        return nullptr;
    }

    TextureViewDescriptor desc = Default;
    desc.label = StringView("Surface texture view");
    desc.dimension = TextureViewDimension::_2D;
    TextureView view = Texture(surface_texture.texture).createView(desc);
    Texture(surface_texture.texture).release();

    return view;
}

void Application::run()
{
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        instance_.processEvents();

        // 1、获取当前要绘制的纹理
        TextureView target_view = get_next_surface_view();
        if (!target_view) return;

        // 2、生成指令生成器，后面的绘制都需要通过这个来创建
        CommandEncoderDescriptor encoder_desc = Default;
        encoder_desc.label = StringView("Command encoder");
        CommandEncoder encoder = device_.createCommandEncoder(encoder_desc);

        RenderPassDescriptor renderpass_desc = Default;
        RenderPassColorAttachment color_attachment = Default;
        color_attachment.view = target_view;
        color_attachment.loadOp = LoadOp::Clear;
        color_attachment.storeOp = StoreOp::Store;
        color_attachment.clearValue = Color{ 1.0, 1.0, 0.0, 1.0 };
        renderpass_desc.colorAttachmentCount = 1;
        renderpass_desc.colorAttachments = &color_attachment;

        RenderPassEncoder renderpass = encoder.beginRenderPass(renderpass_desc);
        renderpass.setPipeline(pipeline_);
        renderpass.draw(3, 1, 0, 0);
        renderpass.end();
        renderpass.release();

        // 3、 生成指令
        CommandBufferDescriptor cmd_buffer_desc = Default;
        cmd_buffer_desc.label = StringView("Command Buffer");
        CommandBuffer command = encoder.finish(cmd_buffer_desc);
        encoder.release();

        // 4、提交指令
        queue_.submit(command);
        command.release();

        target_view.release();

        // 5、呈现
        surface_.present();
    }
}