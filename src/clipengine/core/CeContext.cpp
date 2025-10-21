#include "CeContext.h"
#include "../common/CeHelper.h"

using namespace wgpu;

CeContext::~CeContext() {
    shutdown();
}

bool CeContext::initialize(const CeContextConfig& config) {
    // 初始化 WebGPU
    if (!initializeWebGPU(config)) {
        return false;
    }

    LOG_INFO("CeContext initialized successfully");
    LOG_INFO("Resolution :       {} x {}", native_.width, native_.height);
    LOG_INFO("Surface Format :   {}", static_cast<int>(surface_format_));

    return true;
}

bool CeContext::initializeWebGPU(const CeContextConfig& config) {
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
        #if _WIN32
            native_ = CeHelper::getSurfaceFromWndName(instance_.Get(), config.windowTitle);
        #endif

        if (!native_.surface) {
            LOG_ERROR("Failed to create WebGPU surface");
            return false;
        }

        LOG_INFO("Surface created successfully");
    }

    // 创建 Adapter
    {
        RequestAdapterOptions opts = {
            .powerPreference = PowerPreference::HighPerformance,
            .compatibleSurface = native_.surface
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
        native_.surface.GetCapabilities(adapter_, &caps);

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
            .width = native_.width,
            .height = native_.height,
            .alphaMode = CompositeAlphaMode::Opaque,
            .presentMode = PresentMode::Fifo
        };

        LOG_INFO("Configuring surface: {}x{}, format: {}", native_.width, native_.height, static_cast<int>(surface_format_));
        native_.surface.Configure(&surfaceConfig);
        LOG_INFO("Surface configured successfully");

        // 验证配置：尝试立即获取一次 texture 来测试
        SurfaceTexture testTexture;
        native_.surface.GetCurrentTexture(&testTexture);
        LOG_INFO("Test GetCurrentTexture status: {}", static_cast<int>(testTexture.status));

        if (testTexture.texture) {
            LOG_INFO("Test texture obtained successfully, will be released");
        } else {
            LOG_WARN("Test texture is null, status: {}", static_cast<int>(testTexture.status));
        }
    }

    return true;
}

void CeContext::shutdown() {
    if (native_.surface) {
        native_.surface.Unconfigure();
        native_.surface = nullptr;
    }

    queue_ = nullptr;
    device_ = nullptr;
    adapter_ = nullptr;
    instance_ = nullptr;
}

void CeContext::reconfigureSurface(uint32_t width, uint32_t height) {
    native_.width = width;
    native_.height = height;

    if (!native_.surface || !device_) {
        LOG_ERROR("Cannot reconfigure surface: not initialized");
        return;
    }

    SurfaceConfiguration surfaceConfig = {
        .device = device_,
        .format = surface_format_,
        .usage = TextureUsage::RenderAttachment,
        .width = width,
        .height = height,
        .alphaMode = CompositeAlphaMode::Opaque,
        .presentMode = PresentMode::Fifo
    };
    native_.surface.Configure(&surfaceConfig);

    LOG_INFO("Surface reconfigured to {}x{}", width, height);
}
