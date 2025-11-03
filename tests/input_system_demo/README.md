# InputSystem Demo

这是一个完整的 InputSystem 示例程序，演示了 ClipEngine 窗口系统无关输入系统的所有核心功能。

**位置**: `ClipEngineSDK/samples/input_system_demo/`

## 功能演示

这个 demo 展示了以下 InputSystem 特性：

### ✅ 核心功能
- **事件监听器系统** - 注册和管理多个事件处理器
- **鼠标事件处理** - 点击、拖拽、移动检测
- **键盘事件处理** - 按键检测、组合键支持
- **滚轮事件处理** - 缩放功能演示
- **修饰键追踪** - Ctrl、Shift、Alt、Meta 键状态
- **坐标系统** - 像素坐标和归一化坐标 [0,1]
- **动态事件控制** - 运行时启用/禁用事件类型

### ✅ 窗口系统集成
- **GLFW 适配器** - 自动桥接 GLFW 事件到 InputSystem
- **窗口无关设计** - 核心 InputSystem 不依赖任何窗口库
- **跨平台支持** - Windows、macOS、Linux

### ✅ 实用示例
- **拖拽操作** - 完整的拖拽开始、进行中、结束流程
- **缩放控制** - 鼠标滚轮实现缩放功能
- **快捷键** - Ctrl+S (保存)、Ctrl+Z (撤销) 等
- **WASD 移动** - 游戏风格的键盘控制
- **事件日志** - 实时显示最近的输入事件

## 构建和运行

### 前置要求

- CMake 3.15+
- C++17 编译器
- GLFW 3.x
- OpenGL

### Windows (Visual Studio)

```bash
# 在 ClipEngineSDK/samples/input_system_demo 目录下
cd ClipEngineSDK/samples/input_system_demo
build.bat
run.bat
```

或者手动构建：

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Debug
bin\Debug\input_system_demo.exe
```

### Linux / macOS

```bash
# 在 ClipEngineSDK/samples/input_system_demo 目录下
cd ClipEngineSDK/samples/input_system_demo
chmod +x build.sh run.sh
./build.sh
./run.sh
```

## 使用说明

### 交互控制

运行程序后，窗口会显示一个灰色背景。通过以下操作测试 InputSystem：

| 操作 | 功能 | 事件类型 |
|------|------|----------|
| **鼠标左键点击** | 生成点击事件，显示坐标 | PointerClick |
| **鼠标左键拖拽** | 拖拽操作，显示偏移量 | PointerDown, PointerMove, PointerUp |
| **鼠标移动** | 更新鼠标位置（可切换） | PointerMove |
| **鼠标滚轮** | 缩放功能 (0.1x - 5.0x) | Scroll |
| **W/A/S/D** | 方向键（移动演示） | KeyDown |
| **M 键** | 切换鼠标移动事件的开/关 | KeyDown |
| **Space** | 测试特殊键 | KeyDown |
| **Ctrl+S** | 保存动作演示 | KeyDown (带修饰键) |
| **Ctrl+Z** | 撤销动作演示 | KeyDown (带修饰键) |
| **ESC** | 退出程序 | KeyDown |

### 控制台输出

程序会在控制台实时显示：

1. **详细事件信息** - 每个事件的完整数据
   - 坐标（像素和归一化）
   - 按钮/键码
   - 修饰键状态
   - 时间戳

2. **状态更新** - 每5秒显示一次
   - 当前鼠标位置
   - 缩放级别
   - 总点击次数
   - 拖拽状态
   - 最近事件日志

3. **拖拽追踪** - 拖拽时实时显示偏移量

## 代码结构

```
samples/input_system_demo/
├── main.cpp           # 主程序，包含所有演示代码
├── CMakeLists.txt     # CMake 配置
├── README.md          # 本文档
├── QUICKSTART.md      # 5分钟快速开始指南
├── build.bat          # Windows 构建脚本
├── build.sh           # Linux/macOS 构建脚本
├── run.bat            # Windows 运行脚本
└── run.sh             # Linux/macOS 运行脚本
```

### 代码亮点

#### 1. 事件监听器注册

```cpp
inputSystem.addEventListener(InputEventType::PointerClick, [](const InputEvent& event) {
    std::cout << "Click at: " << event.mouseX << ", " << event.mouseY << std::endl;
});
```

#### 2. 拖拽实现

```cpp
// 开始拖拽
inputSystem.addEventListener(InputEventType::PointerDown, [](const InputEvent& event) {
    if (event.button == MouseButton::Left) {
        isDragging = true;
        dragStartX = event.mouseX;
        dragStartY = event.mouseY;
    }
});

// 结束拖拽
inputSystem.addEventListener(InputEventType::PointerUp, [](const InputEvent& event) {
    isDragging = false;
    float dx = event.mouseX - dragStartX;
    float dy = event.mouseY - dragStartY;
});
```

#### 3. 修饰键检测

```cpp
inputSystem.addEventListener(InputEventType::KeyDown, [](const InputEvent& event) {
    if (event.isCtrlPressed() && event.keyCode == KeyCode::S) {
        std::cout << "Save action!" << std::endl;
    }
});
```

#### 4. 动态事件控制

```cpp
// 按 M 键切换鼠标移动事件
if (event.keyCode == KeyCode::M) {
    mouseMoveEnabled = !mouseMoveEnabled;
    inputSystem.setEventEnabled(InputEventType::PointerMove, mouseMoveEnabled);
}
```

#### 5. GLFW 适配器使用

```cpp
InputSystem inputSystem;
inputSystem.initialize(width, height);

InputSystemGLFWAdapter adapter(&inputSystem);
adapter.attach(glfwWindow);

// 现在 GLFW 事件会自动转发到 InputSystem
```

## 测试要点

运行此 demo 时，请测试以下场景：

### 基础功能
- [ ] 单击鼠标，检查点击事件是否触发
- [ ] 拖拽鼠标，检查拖拽开始/结束事件
- [ ] 滚动鼠标滚轮，检查缩放是否生效
- [ ] 按任意键，检查键盘事件是否触发

### 修饰键
- [ ] 按住 Shift + 点击，检查修饰键状态
- [ ] Ctrl+S，检查组合键识别
- [ ] Ctrl+Z，检查组合键识别

### 特殊功能
- [ ] 按 M 键，检查鼠标移动事件是否正确切换
- [ ] 检查归一化坐标是否在 [0,1] 范围内
- [ ] 检查状态更新是否每5秒显示一次

### 边界情况
- [ ] 快速连续点击，检查事件是否都被捕获
- [ ] 拖拽时移出窗口，检查边界处理
- [ ] 同时按多个修饰键，检查状态追踪

## 学习要点

通过这个 demo，您可以学习：

1. **如何使用 InputSystem** - 从初始化到事件处理的完整流程
2. **适配器模式** - 如何将窗口系统与输入系统解耦
3. **事件驱动编程** - Lambda 回调和事件监听器模式
4. **平台无关设计** - 使用 KeyCode 而非平台特定的键码
5. **实际应用模式** - 拖拽、缩放等常见交互的实现

## 故障排除

### 编译错误

**问题**: 找不到 GLFW
```
Solution: 确保 GLFW 已安装
- Windows: 使用 vcpkg install glfw3
- macOS: brew install glfw
- Linux: sudo apt-get install libglfw3-dev
```

**问题**: 找不到 ClipEngine 头文件
```
Solution: 确保目录结构正确
input_system_demo 应该在 ClipEngineSDK/samples/ 下
```

### 运行时问题

**问题**: 窗口创建失败
```
Solution: 检查 OpenGL 驱动是否安装
确保系统支持 OpenGL 3.3+
```

**问题**: 事件没有触发
```
Solution: 检查控制台输出
确保焦点在窗口上
尝试点击窗口后再测试
```

## 扩展建议

您可以在此 demo 基础上尝试：

1. **添加更多事件类型** - PointerEnter, PointerOut
2. **可视化反馈** - 在窗口中绘制鼠标轨迹
3. **多点触控模拟** - 如果硬件支持
4. **性能测试** - 测试高频事件的处理能力
5. **自定义适配器** - 为其他窗口系统编写适配器

## 相关文档

- [InputSystem API 文档](../../src/clipengine/input/README.md)
- [KeyCode 定义](../../src/clipengine/input/KeyCode.h)
- [GLFW 适配器](../../src/clipengine/input/InputSystemGLFWAdapter.h)
- [Win32 适配器](../../src/clipengine/input/InputSystemWin32Adapter.h)

## 反馈

如果您发现任何问题或有改进建议，欢迎提交 Issue 或 Pull Request。

---

**Happy Testing!** 🎮
