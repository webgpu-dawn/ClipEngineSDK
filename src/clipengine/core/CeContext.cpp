#include "CeContext.h"

#include <vector>
#include <string>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#if _WIN32
#include <Windows.h>
#endif

namespace ClipEngine {

using namespace wgpu;

#if _WIN32
// Windows 平台：通过窗口句柄创建 Surface
WGPUSurface createSurfaceFromHWND(WGPUInstance instance, HWND hwnd) {
    WGPUSurfaceDescriptorFromWindowsHWND fromWindowsHwnd = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceSourceWindowsHWND
        },
        .hinstance = GetModuleHandle(NULL),
        .hwnd = hwnd
    };

    std::string label = "ClipEngine Surface";
    WGPUSurfaceDescriptor desc = {
        .nextInChain = &fromWindowsHwnd.chain,
        .label = {
            .data = label.c_str(),
            .length = label.length()
        }
    };
    return wgpuInstanceCreateSurface(instance, &desc);
}
#endif

CeContext::~CeContext() {
    shutdown();
}

bool CeContext::initialize(const CeContextConfig& config) {
    // 根据模式初始化
    switch (config.mode) {
        case CeContextMode::CREATE_WINDOW:
            if (!createWindowMode(config)) {
                return false;
            }
            break;
        case CeContextMode::FIND_WINDOW:
            if (!findWindowMode(config)) {
                return false;
            }
            break;
        default:
            LOG_ERROR("Unknown context mode");
            return false;
    }

    // 初始化 WebGPU
    if (!initializeWebGPU(config)) {
        return false;
    }

    LOG_INFO("CeContext initialized successfully");
    LOG_INFO("Resolution :       {} x {}", width_, height_);
    LOG_INFO("Surface Format :   {}", static_cast<int>(surface_format_));

    return true;
}

bool CeContext::createWindowMode(const CeContextConfig& config) {
    // 初始化 GLFW
    if (!glfwInit()) {
        LOG_ERROR("Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // 创建窗口
    window_ = glfwCreateWindow(config.width, config.height, config.windowTitle, nullptr, nullptr);
    if (!window_) {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    LOG_INFO("Window created: {} ({}x{})", config.windowTitle, config.width, config.height);
    return true;
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // 窗口消息处理
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool CeContext::findWindowMode(const CeContextConfig& config) {
#if _WIN32
    // // 查找窗口
    // HWND hwnd = FindWindowA(nullptr, config.windowTitle);
    // if (!hwnd) {
    //     LOG_ERROR("Failed to find window: {}", config.windowTitle);
    //     LOG_ERROR("Make sure the window with title '{}' exists", config.windowTitle);
    //     return false;
    // }

    // // 保存窗口句柄供后续使用
    // external_hwnd_ = hwnd;

    // LOG_INFO("Found window with title: {}", config.windowTitle);
    // LOG_INFO("Window handle: 0x{:X}", reinterpret_cast<uintptr_t>(external_hwnd_));

    // // 检查窗口状态
    // if (!IsWindow(hwnd)) {
    //     LOG_ERROR("Invalid window handle");
    //     return false;
    // }

    // if (!IsWindowVisible(hwnd)) {
    //     LOG_WARN("Window is not visible (may be hidden or minimized)");
    // }

    // // 获取窗口状态
    // WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
    // if (GetWindowPlacement(hwnd, &placement)) {
    //     switch (placement.showCmd) {
    //         case SW_SHOWMINIMIZED:
    //             LOG_WARN("Window is minimized");
    //             break;
    //         case SW_SHOWMAXIMIZED:
    //             LOG_INFO("Window is maximized");
    //             break;
    //         case SW_SHOWNORMAL:
    //             LOG_INFO("Window is in normal state");
    //             break;
    //     }
    // }

    // // 获取窗口客户区尺寸
    // RECT rect;
    // if (!GetClientRect(hwnd, &rect)) {
    //     LOG_ERROR("Failed to get client rect for window");
    //     return false;
    // }

    // width_ = rect.right - rect.left;
    // height_ = rect.bottom - rect.top;

    // // 验证尺寸有效
    // if (width_ == 0 || height_ == 0) {
    //     LOG_ERROR("Invalid window size: {}x{}", width_, height_);
    //     LOG_ERROR("The window may not be fully initialized yet or is minimized");
    //     return false;
    // }

    // LOG_INFO("Window client area size: {}x{}", width_, height_);

    // WNDCLASSEX wc = {};
    // wc.cbSize = sizeof(WNDCLASSEX);
    // wc.style = CS_HREDRAW | CS_VREDRAW;
    // wc.lpfnWndProc = WindowProc;
    // wc.hInstance = GetModuleHandle(NULL);
    // wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // wc.lpszClassName = "WebGPUWindowaaa";
    // RegisterClassEx(&wc);

    // // 创建窗口
    // hwnd_ = CreateWindowEx(
    //     0, "WebGPUWindowaaa", "WebGPU Windowaaa",
    //     WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
    //     config.width, config.height, nullptr, nullptr, GetModuleHandle(NULL), this
    // );

    // ShowWindow(hwnd_, SW_SHOW);
    hwnd_ = FindWindow(NULL, "Clipforge");
    // 获取窗口客户区尺寸
    RECT rect;
    if (!GetClientRect(hwnd_, &rect)) {
        LOG_ERROR("Failed to get client rect for window");
        return false;
    }

    width_ = rect.right - rect.left;
    height_ = rect.bottom - rect.top;

    return true;
#else
    LOG_ERROR("FindWindow mode is only supported on Windows");
    return false;
#endif
}

bool CeContext::initializeWebGPU(const CeContextConfig& config) {
    // width_ = config.width;
    // height_ = config.height;

    // 创建 Instance
    {
        constexpr InstanceFeatureName requiredFeatures[] = {
            InstanceFeatureName::TimedWaitAny
        };
        InstanceDescriptor desc = {
            .requiredFeatureCount = std::size(requiredFeatures),
            .requiredFeatures = requiredFeatures
        };
        instance_ = CreateInstance(&desc);
        if (!instance_) {
            LOG_ERROR("Failed to create WebGPU instance");
            return false;
        }
    }

    // 创建 Surface
    {
        switch (config.mode) {
            case CeContextMode::CREATE_WINDOW:
                {
                   surface_ = Surface::Acquire(glfwGetWGPUSurface(instance_.Get(), window_));
                   if (surface_) {
                       surface_.SetLabel("Main Surface");
                   }
                   LOG_INFO("Surface created from GLFW window");
                }
                break;
            case CeContextMode::FIND_WINDOW:
                {
                    #if _WIN32
                        if (!hwnd_) {
                            LOG_ERROR("External window handle is null");
                            return false;
                        }
                        surface_ = Surface::Acquire(createSurfaceFromHWND(instance_.Get(), static_cast<HWND>(hwnd_)));
                        if (surface_) {
                            surface_.SetLabel("Main Surface");
                        }
                    #endif
                }
                break;
            default:
                break;
        }

        if (!surface_) {
            LOG_ERROR("Failed to create WebGPU surface");
            return false;
        }

        LOG_INFO("Surface created successfully");
    }

    // 创建 Adapter
    {
        RequestAdapterOptions opts = {
            .powerPreference = PowerPreference::HighPerformance,
            .compatibleSurface = surface_
        };
        instance_.WaitAny(
            instance_.RequestAdapter(
                &opts, CallbackMode::WaitAnyOnly,
                [&](RequestAdapterStatus status, Adapter adapter, StringView message) {
                    if (status != RequestAdapterStatus::Success) {
                        LOG_ERROR("Failed to get adapter: {}", message.data);
                        return;
                    }
                    adapter_ = std::move(adapter);
                }
            ), UINT64_MAX
        );
        if (!adapter_) {
            LOG_ERROR("Failed to acquire adapter");
            return false;
        }

        // 输出 Adapter 信息
        AdapterInfo adapterInfo;
        adapter_.GetInfo(&adapterInfo);
        LOG_INFO("WebGPU Adapter:");
        LOG_INFO("  Vendor: {}", adapterInfo.vendor.data);
        LOG_INFO("  Device: {}", adapterInfo.device.data);
        LOG_INFO("  Architecture: {}", adapterInfo.architecture.data);
    }

    // 创建 Device
    {
        std::vector<const char*> toggles = {
            "allow_unsafe_apis"
        };
        DawnTogglesDescriptor toggles_desc = {};
        toggles_desc.enabledToggleCount = toggles.size();
        toggles_desc.enabledToggles = toggles.data();

        // 请求的特性
        FeatureName features[] = {
            FeatureName::SharedTextureMemoryDXGISharedHandle,
            FeatureName::DawnMultiPlanarFormats,
            FeatureName::TimestampQuery
        };

        DeviceDescriptor desc = {};
        desc.nextInChain = &toggles_desc;
        desc.requiredFeatureCount = std::size(features);
        desc.requiredFeatures = features;
        desc.defaultQueue.label = "ClipEngine Queue";
        desc.SetUncapturedErrorCallback(
            [](const Device&, ErrorType type, StringView message) {
                LOG_ERROR("WebGPU Uncaptured Error: {}", message.data);
            }
        );

        instance_.WaitAny(
            adapter_.RequestDevice(
                &desc, CallbackMode::WaitAnyOnly,
                [&](RequestDeviceStatus status, Device device, StringView message) {
                    if (status != RequestDeviceStatus::Success) {
                        LOG_ERROR("Failed to get device: {}", message.data);
                        return;
                    }
                    device_ = std::move(device);
                    queue_ = device_.GetQueue();
                }
            ), UINT64_MAX
        );

        if (!device_) {
            LOG_ERROR("Failed to acquire device");
            return false;
        }
    }

    // 配置 Surface
    {
        SurfaceCapabilities caps;
        surface_.GetCapabilities(adapter_, &caps);

        LOG_INFO("Surface capabilities:");
        LOG_INFO("  - Format count: {}", caps.formatCount);
        LOG_INFO("  - Alpha mode count: {}", caps.alphaModeCount);
        LOG_INFO("  - Present mode count: {}", caps.presentModeCount);

        if (caps.formatCount == 0) {
            LOG_ERROR("No supported surface formats available");
            return false;
        }

        surface_format_ = caps.formats[0];
        LOG_INFO("  - Selected format: {}", static_cast<int>(surface_format_));

        SurfaceConfiguration surfaceConfig = {
            .device = device_,
            .format = surface_format_,
            .usage = TextureUsage::RenderAttachment,
            .width = width_,
            .height = height_,
            .alphaMode = CompositeAlphaMode::Opaque,
            .presentMode = PresentMode::Fifo
        };

        LOG_INFO("Configuring surface: {}x{}, format: {}", width_, height_, static_cast<int>(surface_format_));
        surface_.Configure(&surfaceConfig);
        LOG_INFO("Surface configured successfully");

        // 验证配置：尝试立即获取一次 texture 来测试
        SurfaceTexture testTexture;
        surface_.GetCurrentTexture(&testTexture);
        LOG_INFO("Test GetCurrentTexture status: {}", static_cast<int>(testTexture.status));

        if (testTexture.texture) {
            LOG_INFO("Test texture obtained successfully, will be released");
            // 不需要手动释放，Present() 会处理
        } else {
            LOG_WARN("Test texture is null, status: {}", static_cast<int>(testTexture.status));
        }
    }

    return true;
}

void CeContext::shutdown() {
    if (surface_) {
        surface_.Unconfigure();
        surface_ = nullptr;
    }

    queue_ = nullptr;
    device_ = nullptr;
    adapter_ = nullptr;
    instance_ = nullptr;

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

void CeContext::reconfigureSurface(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    if (!surface_ || !device_) {
        LOG_ERROR("Cannot reconfigure surface: not initialized");
        return;
    }

    SurfaceConfiguration surfaceConfig = {
        .device = device_,
        .format = surface_format_,
        .usage = TextureUsage::RenderAttachment,
        .width = width_,
        .height = height_,
        .alphaMode = CompositeAlphaMode::Opaque,
        .presentMode = PresentMode::Fifo
    };
    surface_.Configure(&surfaceConfig);

    LOG_INFO("Surface reconfigured to {}x{}", width_, height_);
}

} // namespace ClipEngine
