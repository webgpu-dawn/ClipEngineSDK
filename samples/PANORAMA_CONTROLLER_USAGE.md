# PanoramaController Usage

`PanoramaController` 封装了全景视频的交互逻辑，使示例代码更简洁。

## 功能

### 鼠标交互
- **拖拽旋转**: 左键拖动可旋转全景视频
- **缩放**: 鼠标滚轮可缩放视图

### 键盘快捷键 (颜色调整)
- **Q/W**: 增加/减少亮度
- **A/S**: 增加/减少对比度
- **Z/X**: 增加/减少曝光
- **C/V**: 增加/减少增益
- **R**: 重置所有颜色调整

## 使用方法

### 1. 在 Application.h 中添加

```cpp
#include "PanoramaController.h"

class Application {
    ...
private:
    PanoramaController panoramaController_;
};
```

### 2. 在 Application.cpp 的 setupInputEventListeners() 中初始化

```cpp
void Application::setupInputEventListeners()
{
    // 初始化 PanoramaController
    panoramaController_.initialize(videoLayer_, colorEffect_);

    // 注册所有交互事件监听器
    panoramaController_.registerInputListeners(inputSystem_);

    // 然后注册 Application 特定的事件
    // (如模式切换、导出、退出等)
    inputSystem_.addEventListener(InputEventType::KeyDown,
        [this](const InputEvent& e) { handleKeyDown(e); });
}
```

### 3. Application 只需处理应用级别的功能

Application.cpp 中只保留:
- 模式切换 (1-4 键)
- 导出视频 (E 键)
- 退出程序 (ESC 键)
- 加载图片 (L 键)
- 切换图片可见性 (I 键)

所有全景旋转、缩放、颜色调整都由 `PanoramaController` 自动处理。

## 好处

1. **代码更简洁**: Application 类从 ~500 行减少到 ~300 行
2. **职责分离**: 交互逻辑独立于应用逻辑
3. **易于复用**: PanoramaController 可以在其他项目中直接使用
4. **易于维护**: 交互相关的bug修复集中在一个类中
5. **易于扩展**: 可以轻松添加新的交互方式或快捷键

## 实现细节

`PanoramaController` 管理以下状态:
- 全景旋转 (yaw, pitch, zoom)
- 拖拽状态 (dragging, lastMouseX/Y)
- 颜色参数 (brightness, contrast, exposure, gain)

所有状态更新会自动同步到 VideoRenderer 和 ShaderEffect。
