# ClipEngine Logger 设置说明

## 安装 spdlog

ClipEngine 使用 spdlog 作为日志库。需要通过 vcpkg 安装：

```bash
# 安装 spdlog
vcpkg install spdlog:x64-windows
```

如果已经安装，请确保 vcpkg 集成已启用：

```bash
vcpkg integrate install
```

## Logger 使用示例

### 基本使用

```cpp
#include <clipengine/ClipEngine.h>

int main() {
    // 基本日志输出
    LOG_INFO("ClipEngine version: {}", ClipEngine::getVersion());
    LOG_DEBUG("Debug message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    
    return 0;
}
```

### 设置日志级别

```cpp
#include <clipengine/util/Logger.h>

int main() {
    // 设置日志级别为 debug（显示所有日志）
    ClipEngine::Logger::instance().setLevel(spdlog::level::debug);
    
    // 或者设置为 warn（只显示警告和错误）
    ClipEngine::Logger::instance().setLevel(spdlog::level::warn);
    
    LOG_TRACE("This won't show if level is info or higher");
    LOG_DEBUG("This won't show if level is info or higher");
    LOG_INFO("This will show");
    LOG_WARN("This will show");
    LOG_ERROR("This will show");
    
    return 0;
}
```

### 自定义日志格式

```cpp
#include <clipengine/util/Logger.h>

int main() {
    // 自定义格式
    // %H:%M:%S - 时间
    // %l - 日志级别
    // %v - 消息内容
    // %s - 源文件名
    // %# - 行号
    ClipEngine::Logger::instance().setPattern("[%H:%M:%S] [%^%l%$] [%s:%#] %v");
    
    LOG_INFO("Formatted log message");
    
    return 0;
}
```

### 格式化字符串

```cpp
#include <clipengine/ClipEngine.h>

int main() {
    int width = 1920;
    int height = 1080;
    
    // 使用 fmt/spdlog 格式化语法
    LOG_INFO("Resolution: {}x{}", width, height);
    LOG_DEBUG("Float value: {:.2f}", 3.14159);
    LOG_WARN("Hex value: 0x{:X}", 255);
    
    return 0;
}
```

## 日志级别说明

- **TRACE** (灰色) - 详细的调试信息
- **DEBUG** (青色) - 调试信息
- **INFO** (绿色) - 一般信息（默认级别）
- **WARN** (黄色) - 警告信息
- **ERROR** (红色) - 错误信息
- **CRITICAL** (洋红色) - 严重错误

## 颜色输出

Logger 默认启用彩色输出，支持：
- Windows 10+ (通过 ANSI 转义码)
- Linux/macOS 终端
- 大部分现代 IDE 的输出窗口

如果终端不支持颜色，可以通过 spdlog 配置禁用。
