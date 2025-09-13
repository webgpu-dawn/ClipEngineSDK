#include "application.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <glfw3webgpu.h>

using namespace wgpu;
using namespace std;

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
        WGPUDevice const * device,
		WGPUDeviceLostReason reason,
		struct WGPUStringView message,
		void* /* userdata1 */,
		void* /* userdata2 */
    ){
        std::cout
	    	<< "Device " << device << " was lost: reason " << reason
	    	<< " (" << StringView(message) << ")" // NEW
	    	<< std::endl;
    };
    device_desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    device_desc.uncapturedErrorCallbackInfo.callback = []( // TODO: setter
		WGPUDevice const * device,
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
    if(status != Status::Success) return false;

    SurfaceConfiguration config = Default;
    config.width = 640;
    config.height= 480;
    config.device = device_;
    config.format = capabilities.formats[0];
    config.presentMode = PresentMode::Fifo;
    config.alphaMode   = CompositeAlphaMode::Auto;
    surface_.configure(config);

    capabilities.freeMembers();
    adapter.release();

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
    if(
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
    std::cout << "Starting render loop..." << std::endl;
    while(!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        instance_.processEvents();

        // 1、获取当前要绘制的纹理
        TextureView target_view = get_next_surface_view();
        if(!target_view) return;

        // 2、生成指令生成器，后面的绘制都需要通过这个来创建
        CommandEncoderDescriptor encoder_desc = Default;
        encoder_desc.label = StringView("Command encoder");
        CommandEncoder encoder = device_.createCommandEncoder(encoder_desc);

        RenderPassDescriptor renderpass_desc = Default;
        RenderPassColorAttachment color_attachment = Default;
        color_attachment.view = target_view;
        color_attachment.loadOp = LoadOp::Clear;
        color_attachment.storeOp = StoreOp::Store;
        color_attachment.clearValue = Color { 1.0, 1.0, 0.0, 1.0 };
        renderpass_desc.colorAttachmentCount = 1;
        renderpass_desc.colorAttachments = &color_attachment;

        RenderPassEncoder renderpass = encoder.beginRenderPass(renderpass_desc);
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