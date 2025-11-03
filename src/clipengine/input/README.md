# ClipEngine InputSystem

统一的输入系统，用于处理鼠标、触摸和键盘事件。设计灵感来自 Orillusion 的 InputSystem。

## 核心特性

- ✅ **完全窗口系统无关**：不依赖 GLFW、Win32、SDL 或任何特定窗口系统
- ✅ 统一的事件模型（鼠标和触摸使用相同接口）
- ✅ 事件监听器模式（基于回调）
- ✅ 修饰键追踪（Shift, Ctrl, Alt, Meta）
- ✅ 多点触控支持（通过 pointerId）
- ✅ 标准化坐标（像素和归一化 [0,1]）
- ✅ 运动跟踪（movementX/Y）
- ✅ 滚轮支持
- ✅ 线程安全的监听器管理
- ✅ 提供适配器支持多种窗口系统（GLFW, Win32, SDL, Qt 等）

## 架构设计

InputSystem 采用**事件注入**模式，核心系统不依赖任何窗口库：

```
┌──────────────────────────────────────┐
│      Application / Window System     │
│      (GLFW / Win32 / SDL / Qt)       │
└──────────────┬───────────────────────┘
               │ Window Events
               ▼
┌──────────────────────────────────────┐
│       Window System Adapter          │
│  (GLFWAdapter / Win32Adapter / ...)  │
└──────────────┬───────────────────────┘
               │ inject*() calls
               ▼
┌──────────────────────────────────────┐
│         InputSystem (Core)           │
│      - Event dispatch                │
│      - Listener management           │
│      - State tracking                │
└──────────────┬───────────────────────┘
               │ Callbacks
               ▼
┌──────────────────────────────────────┐
│      Your Event Handlers             │
└──────────────────────────────────────┘
```

## 支持的事件类型

| 事件类型 | 说明 |
|---------|------|
| `PointerClick` | 鼠标点击或触摸点击 |
| `PointerDown` | 鼠标按下或触摸开始 |
| `PointerUp` | 鼠标释放或触摸结束 |
| `PointerMove` | 鼠标移动或触摸滑动 |
| `PointerOut` | 鼠标离开区域或触摸取消 |
| `PointerEnter` | 鼠标进入区域 |
| `KeyDown` | 键盘按键按下 |
| `KeyUp` | 键盘按键释放 |
| `Scroll` | 鼠标滚轮或触控板滚动 |

## 快速开始

### 使用 GLFW 适配器

```cpp
#include <clipengine/input/InputSystem.h>
#include <clipengine/input/InputSystemGLFWAdapter.h>
#include <clipengine/input/KeyCode.h>
#include <GLFW/glfw3.h>

using namespace clipengine;

// 创建输入系统
InputSystem inputSystem;
inputSystem.initialize(1920, 1080);

// 使用 GLFW 适配器
InputSystemGLFWAdapter adapter(&inputSystem);
adapter.attach(glfwWindow);

// 注册事件监听器
inputSystem.addEventListener(
    InputEventType::PointerClick,
    [](const InputEvent& event) {
        std::cout << "Click at: " << event.mouseX << ", " << event.mouseY << std::endl;
    }
);

// 主循环
while (!glfwWindowShouldClose(window)) {
    inputSystem.update(deltaTime);
    glfwPollEvents();
}

// 清理
adapter.detach();
inputSystem.shutdown();
```

### 使用 Win32 适配器

```cpp
#include <clipengine/input/InputSystem.h>
#include <clipengine/input/InputSystemWin32Adapter.h>
#include <clipengine/input/KeyCode.h>
#include <windows.h>

using namespace clipengine;

InputSystem inputSystem;
InputSystemWin32Adapter adapter(&inputSystem);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 让适配器处理输入消息
    if (adapter.handleMessage(msg, wParam, lParam)) {
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 注册事件监听器
inputSystem.addEventListener(InputEventType::KeyDown, [](const InputEvent& e) {
    if (e.keyCode == KeyCode::Space) {
        std::cout << "Space pressed!" << std::endl;
    }
});
```

### 直接使用事件注入 API（无窗口系统）

如果您使用的窗口系统没有现成的适配器，可以直接调用事件注入 API：

```cpp
#include <clipengine/input/InputSystem.h>
#include <clipengine/input/KeyCode.h>

using namespace clipengine;

InputSystem inputSystem;
inputSystem.initialize(1920, 1080);

// 在窗口系统的回调中，手动注入事件
void onMouseMove(float x, float y) {
    inputSystem.injectMouseMove(x, y);
}

void onMouseButton(int button, bool pressed) {
    ModifierKeys modifiers;
    modifiers.shift = /* 检查 Shift 键状态 */;
    modifiers.ctrl = /* 检查 Ctrl 键状态 */;
    inputSystem.injectMouseButton(button, pressed, modifiers);
}

void onKeyEvent(int keyCode, int scanCode, bool pressed) {
    ModifierKeys modifiers;
    // ... 设置修饰键
    inputSystem.injectKey(keyCode, scanCode, pressed, modifiers);
}

void onScroll(float deltaX, float deltaY) {
    inputSystem.injectScroll(deltaX, deltaY);
}
```

## 使用通用键码

我们提供了 `KeyCode.h`，定义了与窗口系统无关的常用键码：

```cpp
#include <clipengine/input/KeyCode.h>

inputSystem.addEventListener(InputEventType::KeyDown, [](const InputEvent& event) {
    if (event.keyCode == KeyCode::W) {
        moveForward();
    }

    if (event.keyCode == KeyCode::Space) {
        jump();
    }

    if (event.isCtrlPressed() && event.keyCode == KeyCode::S) {
        save();
    }
});
```

**注意**：您也可以直接使用平台特定的键码（如 `GLFW_KEY_W` 或 `VK_W`），适配器会自动传递它们。

## 事件处理示例

### 处理鼠标移动

```cpp
inputSystem.addEventListener(InputEventType::PointerMove, [](const InputEvent& event) {
    std::cout << "Mouse: " << event.mouseX << ", " << event.mouseY << std::endl;
    std::cout << "Delta: " << event.movementX << ", " << event.movementY << std::endl;
    std::cout << "Normalized: " << event.normalizedX << ", " << event.normalizedY << std::endl;
});
```

### 处理鼠标按钮

```cpp
inputSystem.addEventListener(InputEventType::PointerDown, [](const InputEvent& event) {
    switch (event.button) {
        case MouseButton::Left:
            std::cout << "Left button pressed" << std::endl;
            break;
        case MouseButton::Right:
            std::cout << "Right button pressed" << std::endl;
            break;
        case MouseButton::Middle:
            std::cout << "Middle button pressed" << std::endl;
            break;
    }
});
```

### 处理键盘输入

```cpp
inputSystem.addEventListener(InputEventType::KeyDown, [](const InputEvent& event) {
    if (event.keyCode == KeyCode::Escape) {
        exit();
    }

    // 检查修饰键
    if (event.isShiftPressed()) {
        std::cout << "Shift is held" << std::endl;
    }

    // 组合键
    if (event.isCtrlPressed() && event.keyCode == KeyCode::C) {
        copy();
    }
});
```

### 处理滚轮

```cpp
inputSystem.addEventListener(InputEventType::Scroll, [](const InputEvent& event) {
    float zoom = 1.0f;
    zoom += event.deltaY * 0.1f;
    std::cout << "Zoom: " << zoom << std::endl;
});
```

## 查询输入状态

```cpp
// 获取鼠标位置
float mouseX, mouseY;
inputSystem.getMousePosition(mouseX, mouseY);

// 获取归一化坐标 [0, 1]
float normX, normY;
inputSystem.getMousePositionNormalized(normX, normY);

// 获取修饰键状态
const auto& modifiers = inputSystem.getModifierKeys();
if (modifiers.shift) {
    std::cout << "Shift is pressed" << std::endl;
}
```

## 控制事件启用/禁用

```cpp
// 禁用鼠标移动事件（提高性能）
inputSystem.setEventEnabled(InputEventType::PointerMove, false);

// 重新启用
inputSystem.setEventEnabled(InputEventType::PointerMove, true);

// 检查是否启用
if (inputSystem.isEventEnabled(InputEventType::PointerClick)) {
    std::cout << "Click events are enabled" << std::endl;
}
```

## 高级用法

### 实现拖拽功能

```cpp
bool isDragging = false;
float dragStartX = 0.0f;
float dragStartY = 0.0f;

inputSystem.addEventListener(InputEventType::PointerDown, [&](const InputEvent& event) {
    if (event.button == MouseButton::Left) {
        isDragging = true;
        dragStartX = event.mouseX;
        dragStartY = event.mouseY;
    }
});

inputSystem.addEventListener(InputEventType::PointerMove, [&](const InputEvent& event) {
    if (isDragging) {
        float dx = event.mouseX - dragStartX;
        float dy = event.mouseY - dragStartY;
        // 更新对象位置...
    }
});

inputSystem.addEventListener(InputEventType::PointerUp, [&](const InputEvent& event) {
    if (event.button == MouseButton::Left) {
        isDragging = false;
    }
});
```

### 实现缩放功能

```cpp
float zoom = 1.0f;

inputSystem.addEventListener(InputEventType::Scroll, [&](const InputEvent& event) {
    const float zoomSpeed = 0.1f;
    zoom += event.deltaY * zoomSpeed;
    zoom = std::max(0.1f, std::min(10.0f, zoom));  // 限制范围
});
```

### 多监听器管理

```cpp
std::vector<int> listenerIds;

// 注册多个监听器
listenerIds.push_back(inputSystem.addEventListener(InputEventType::PointerClick, callback1));
listenerIds.push_back(inputSystem.addEventListener(InputEventType::PointerMove, callback2));
listenerIds.push_back(inputSystem.addEventListener(InputEventType::KeyDown, callback3));

// 清理所有监听器
for (int id : listenerIds) {
    inputSystem.removeEventListener(id);
}

// 或者移除特定类型的所有监听器
inputSystem.removeAllListeners(InputEventType::PointerMove);

// 或者移除所有监听器
inputSystem.removeAllListeners();
```

## 创建自定义适配器

如果您使用的窗口系统没有现成的适配器，可以轻松创建一个：

```cpp
#include <clipengine/input/InputSystem.h>
#include <your_window_system.h>

class InputSystemYourAdapter {
public:
    explicit InputSystemYourAdapter(InputSystem* inputSystem)
        : inputSystem_(inputSystem) {}

    void onMouseMove(float x, float y) {
        inputSystem_->injectMouseMove(x, y);
    }

    void onMouseButton(int button, bool pressed) {
        ModifierKeys mods;
        // 从窗口系统获取修饰键状态
        mods.shift = /* ... */;
        mods.ctrl = /* ... */;
        inputSystem_->injectMouseButton(button, pressed, mods);
    }

    void onKey(int keyCode, int scanCode, bool pressed) {
        ModifierKeys mods = getModifierKeys();
        inputSystem_->injectKey(keyCode, scanCode, pressed, mods);
    }

    void onScroll(float deltaX, float deltaY) {
        inputSystem_->injectScroll(deltaX, deltaY);
    }

private:
    InputSystem* inputSystem_;
};
```

## InputEvent 对象属性

```cpp
struct InputEvent {
    // 事件类型和设备
    InputEventType type;
    InputDeviceType deviceType;

    // 指针标识（多点触控）
    int pointerId;

    // 屏幕坐标（像素）
    float mouseX, mouseY;

    // 归一化坐标 [0, 1]
    float normalizedX, normalizedY;

    // 移动增量
    float movementX, movementY;

    // 滚轮增量
    float deltaX, deltaY;

    // 鼠标按钮
    MouseButton button;
    bool buttonPressed;

    // 键盘状态
    int keyCode;     // 平台键码 (GLFW, Win32 VK_*, SDL, 或 KeyCode::*)
    int scanCode;    // 平台扫描码

    // 修饰键
    ModifierKeys modifiers;

    // 触摸压力 [0.0 - 1.0]
    float pressure;

    // 时间戳
    double timestamp;

    // 用户数据（可扩展）
    void* userData;

    // 辅助方法
    bool isShiftPressed() const;
    bool isCtrlPressed() const;
    bool isAltPressed() const;
    bool isMetaPressed() const;
};
```

## 与 Orillusion 的对比

| Orillusion | ClipEngine | 说明 |
|------------|------------|------|
| `Engine3D.inputSystem.addEventListener()` | `inputSystem.addEventListener()` | 注册事件监听器 |
| `PointerEvent3D.POINTER_CLICK` | `InputEventType::PointerClick` | 点击事件 |
| `event.target` | `event.userData` | 自定义数据（可扩展） |
| `event.data.Normal/Position` | 可通过 userData 扩展 | 3D 坐标信息 |

## 性能建议

1. **禁用不需要的事件**：如果不需要 `PointerMove` 事件，禁用它可以提高性能
   ```cpp
   inputSystem.setEventEnabled(InputEventType::PointerMove, false);
   ```

2. **避免在事件回调中执行耗时操作**：事件回调应该快速返回，将耗时操作放到主循环中

3. **及时移除不需要的监听器**：不再使用的监听器应该及时移除

## 线程安全

InputSystem 的监听器管理是线程安全的，可以从不同线程添加/移除监听器。事件回调通常在窗口事件线程中执行。

## 支持的窗口系统

- ✅ **GLFW** - `InputSystemGLFWAdapter.h`
- ✅ **Win32** - `InputSystemWin32Adapter.h`
- 🔄 **SDL** - 可以轻松创建适配器
- 🔄 **Qt** - 可以轻松创建适配器
- 🔄 **其他** - 通过事件注入 API 支持任何窗口系统

## License

Part of ClipEngine SDK
