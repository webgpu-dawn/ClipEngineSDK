# 视频分辨率变化处理

## 概述

ClipEngine 的 `VideoRenderer` 已经支持**自动处理视频分辨率变化**。当输入视频帧的分辨率改变时，引擎会自动：

1. 检测分辨率变化
2. 清理旧的纹理资源
3. 重新创建适配新分辨率的纹理
4. 继续正常渲染

## 自动处理机制

### 分辨率检测

在 `VideoRenderer::updateFrame()` 中，每次更新帧时都会检查分辨率：

```cpp
// 检测分辨率是否改变
bool resolutionChanged = (sharedTextureData_.width != srcDesc.Width ||
                          sharedTextureData_.height != srcDesc.Height);
```

### 资源清理

当检测到分辨率变化时，会自动清理旧资源：

```cpp
if (resolutionChanged) {
    std::cout << "[VideoRenderer] Resolution changed: "
              << sharedTextureData_.width << "x" << sharedTextureData_.height
              << " -> " << srcDesc.Width << "x" << srcDesc.Height << std::endl;
    cleanupSharedTextures();  // 清理旧纹理
}
```

`cleanupSharedTextures()` 会：
- 结束 Dawn 纹理的访问（EndAccess）
- 释放共享内存
- 关闭共享句柄
- 清除缓存的 TextureView

### 纹理重建

清理完成后，会使用新分辨率重新创建纹理：

```cpp
// 重新创建 D3D11 共享纹理
if (!CreateD3D11SharedTexture(d3d11Device, srcDesc, sharedTextureData_)) {
    return false;
}

// 重新导入到 Dawn
dawnTextureData_ = ImportToDawnTexture(device_, sharedTextureData_.handle);
```

## 使用示例

### 基本用法

```cpp
// 创建视频渲染器
auto videoRenderer = std::make_unique<VideoRenderer>();

// 视频帧可能有不同分辨率 - 引擎会自动处理
ID3D11Texture2D* frame1 = ...; // 1920x1080
videoRenderer->updateFrame(frame1);

ID3D11Texture2D* frame2 = ...; // 3840x2160 - 分辨率变化！
videoRenderer->updateFrame(frame2);  // 自动检测并处理

ID3D11Texture2D* frame3 = ...; // 1280x720 - 又变化了！
videoRenderer->updateFrame(frame3);  // 继续正常工作
```

### 监听分辨率变化

分辨率变化时会输出日志：

```
[VideoRenderer] Resolution changed: 1920x1080 -> 3840x2160
```

### 性能考虑

虽然分辨率变化会自动处理，但频繁变化会影响性能，因为：

1. **纹理重建开销** - 需要重新分配GPU内存
2. **管线同步** - 需要等待GPU完成当前操作
3. **资源清理** - 需要释放旧资源

**建议**：
- 尽量避免频繁的分辨率切换
- 如果知道视频有多种分辨率，考虑预先创建多个 VideoRenderer 实例
- 使用固定分辨率的输出可以提高性能

## 与多实例的兼容性

### ⚠️ 重要改进

**旧版本问题**：使用静态变量存储纹理数据，多个 VideoRenderer 实例会相互干扰。

**新版本**：每个 VideoRenderer 实例都有独立的纹理数据：

```cpp
class VideoRenderer {
    SharedTextureData sharedTextureData_;  // 实例成员，非静态
    DawnTextureData dawnTextureData_;      // 实例成员，非静态
};
```

现在可以安全地使用多个实例：

```cpp
auto video1 = std::make_unique<VideoRenderer>();  // 1080p 视频
auto video2 = std::make_unique<VideoRenderer>();  // 4K 视频
auto video3 = std::make_unique<VideoRenderer>();  // 720p 视频

// 它们互不干扰，各自管理自己的纹理
video1->updateFrame(frame1080p);
video2->updateFrame(frame4K);
video3->updateFrame(frame720p);
```

## 高级场景

### 场景1：自适应视频流

处理网络流媒体的自适应码率切换：

```cpp
class AdaptiveVideoPlayer {
    VideoRenderer renderer_;

    void onBitrateChange(int newBitrate) {
        // 切换到不同分辨率的流
        // VideoRenderer 会自动处理分辨率变化
        if (newBitrate > 5000000) {
            currentStream_ = stream4K_;   // 3840x2160
        } else if (newBitrate > 2000000) {
            currentStream_ = stream1080p_; // 1920x1080
        } else {
            currentStream_ = stream720p_;  // 1280x720
        }
    }
};
```

### 场景2：多视频源混合

在同一场景中混合不同分辨率的视频：

```cpp
CompositionEngine engine;

// 主视频 - 4K
auto mainVideo = std::make_unique<VideoRenderer>();
mainVideo->setTransform(0.0f, 0.0f, 1.0f, 1.0f);
engine.addLayer(std::move(mainVideo));

// 画中画 - 720p
auto pipVideo = std::make_unique<VideoRenderer>();
pipVideo->setTransform(0.7f, 0.7f, 0.3f, 0.3f);
engine.addLayer(std::move(pipVideo));

// 每个视频独立管理自己的分辨率
```

### 场景3：导出时的分辨率处理

导出视频时，可能需要调整分辨率：

```cpp
void exportVideo(VideoSource* source, int targetWidth, int targetHeight) {
    VideoExportConfig config = {
        .width = targetWidth,
        .height = targetHeight,
        // ...
    };

    VideoExporter exporter;
    exporter.initialize(config, device);

    while (!exporter.isFinished()) {
        auto frame = source->getFrame();
        // VideoRenderer 会处理源分辨率到目标分辨率的转换
        renderer->updateFrame(frame.texture);
        exporter.exportFrame();
    }
}
```

## 故障排查

### 问题：分辨率变化后渲染出错

**可能原因**：
1. 纹理格式不匹配
2. GPU 内存不足
3. 共享纹理创建失败

**解决方法**：
```cpp
if (!videoRenderer->updateFrame(texture)) {
    std::cerr << "Failed to update frame - check logs for details" << std::endl;
    // 检查日志中的错误信息
    // 可能需要减小视频分辨率或释放其他 GPU 资源
}
```

### 问题：性能下降

如果频繁变化分辨率导致性能问题，可以考虑：

1. **使用固定分辨率** - 在源头统一分辨率
2. **预分配纹理池** - 预先创建常用分辨率的纹理
3. **批处理变化** - 累积多帧后再切换

## API 参考

### cleanupSharedTextures()

```cpp
void VideoRenderer::cleanupSharedTextures();
```

手动清理共享纹理资源。通常不需要手动调用，因为：
- 析构函数会自动调用
- `updateFrame()` 在检测到分辨率变化时自动调用

### 内部结构

```cpp
struct SharedTextureData {
    ComPtr<ID3D11Texture2D> texture;
    HANDLE handle;
    uint32_t width;
    uint32_t height;
};

struct DawnTextureData {
    wgpu::Texture texture;
    wgpu::SharedTextureMemory sharedMemory;
    uint32_t width;
    uint32_t height;
};
```

这些结构用于跟踪当前纹理状态，每个 VideoRenderer 实例都有独立的副本。

## 总结

✅ **自动处理** - 无需手动管理分辨率变化
✅ **资源安全** - 正确清理和重建纹理
✅ **多实例支持** - 每个 VideoRenderer 独立管理
✅ **性能优化** - 只在必要时重建纹理
✅ **日志记录** - 输出分辨率变化信息便于调试
