# InputSystem Demo - 快速开始

## 5 分钟快速测试

### Windows 用户

1. **编译**（如果还没编译）
   ```bash
   cd ClipEngineSDK/samples/input_system_demo
   build.bat
   ```

2. **运行**
   ```bash
   run.bat
   ```
   或者直接运行：
   ```bash
   build\bin\Debug\input_system_demo.exe
   ```

3. **测试**
   - 点击鼠标 → 查看控制台输出点击事件
   - 拖拽鼠标 → 查看拖拽偏移量
   - 滚动鼠标滚轮 → 查看缩放变化
   - 按 W/A/S/D → 查看方向键事件
   - 按 M 键 → 切换鼠标移动事件
   - Ctrl+S → 查看组合键识别
   - ESC → 退出

### Linux / macOS 用户

1. **编译**
   ```bash
   cd ClipEngineSDK/samples/input_system_demo
   chmod +x build.sh run.sh
   ./build.sh
   ```

2. **运行**
   ```bash
   ./run.sh
   ```

3. **测试**（同上）

## 预期输出示例

```
========================================
  InputSystem Demo - Interactive Test
========================================

Controls:
  Mouse Left Click    - Generate click event
  Mouse Left Drag     - Drag operation
  Mouse Wheel         - Zoom in/out
  ...

Window created. Start interacting!
Press ESC to quit.

=== POINTER CLICK ===
  Button: Left
  Position: (324, 156)
  Normalized: (0.405, 0.260)
  Modifiers: None
  Total clicks: 1

--- Drag Started ---

  Dragging... offset: (45, -23)

--- Drag Ended ---
  Drag delta: (45, -23)

=== SCROLL ===
  Delta: (0, 1.2)
  New zoom: 1.12x

=== KEY DOWN ===
  Key Code: 87
  Scan Code: 17
  Modifiers: None
  >> W - Move forward
```

## 常见问题

**Q: 编译失败，找不到 GLFW**
A: 确保已安装 GLFW。Windows 上使用 `vcpkg install glfw3`

**Q: 窗口无法创建**
A: 检查 OpenGL 驱动是否正确安装

**Q: 没有任何输出**
A: 确保点击了窗口，窗口获得焦点后再测试

**Q: 想要修改代码测试**
A: 编辑 `main.cpp`，然后重新运行 build.bat

## 下一步

- 阅读完整文档：[README.md](README.md)
- 查看 InputSystem API：[../../src/clipengine/input/README.md](../../src/clipengine/input/README.md)
- 学习如何集成到您的项目中

## 测试清单

- [ ] 鼠标点击 ✓
- [ ] 鼠标拖拽 ✓
- [ ] 滚轮缩放 ✓
- [ ] 键盘输入 ✓
- [ ] 修饰键 (Ctrl/Shift/Alt) ✓
- [ ] 组合键 (Ctrl+S) ✓
- [ ] M 键切换事件 ✓
- [ ] ESC 退出 ✓
