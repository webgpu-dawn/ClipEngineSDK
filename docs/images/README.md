# ClipEngine 图片资源说明

本目录包含 ClipEngine 文档所需的所有图片资源。

## 📋 图片清单

### ✅ 已创建的 SVG 图片

所有图片均已创建为 SVG 矢量图格式，可在任何分辨率下保持清晰。

| 文件名 | 尺寸 | 用途 | 状态 |
|--------|------|------|------|
| `clipengine-logo.svg` | 400x120 | 项目 Logo | ✅ 2.3KB |
| `architecture.svg` | 900x600 | 系统架构图 | ✅ 8.3KB |
| `pipeline-overview.svg` | 800x500 | 渲染管线概览 | ✅ 7.2KB |
| `color-management.svg` | 700x400 | 色彩管理流程 | ✅ 7.5KB |
| `render-performance.svg` | 650x400 | 性能对比图表 | ✅ 6.6KB |
| `effects-showcase.svg` | 800x600 | 效果展示合集 | ✅ 8.8KB |
| `preview-window.svg` | 700x500 | 预览窗口界面 | ✅ 7.3KB |
| `composition-demo.svg` | 700x400 | 合成系统演示 | ✅ 4.5KB |
| `video-effects-demo.svg` | 750x420 | 视频特效演示 | ✅ 5.2KB |
| `case-video-editor.svg` | 250x180 | 视频编辑器案例 | ✅ 2.4KB |
| `case-live-streaming.svg` | 250x180 | 直播特效案例 | ✅ 3.4KB |
| `case-motion-graphics.svg` | 250x180 | 动态图形案例 | ✅ 4.2KB |

**总计**: 12 个 SVG 文件，总大小约 67.7KB

### 🎬 GIF 动画替代方案

由于 GIF 需要实际录制程序运行画面，目前提供的 SVG 版本为静态示意图。如需创建真实的 GIF 动画：

#### 录制 composition-demo.gif
1. 运行 Sample_sdk_demo
2. 演示多图层合成过程
3. 使用 ScreenToGif 录制
4. 建议时长：5-10 秒

#### 录制 video-effects-demo.gif
1. 运行 Sample_sdk_demo
2. 实时调整效果参数（亮度、饱和度等）
3. 展示参数变化对视频的影响
4. 建议时长：5-10 秒

### 📸 PNG/JPG 导出（可选）

如需将 SVG 转换为 PNG/JPG 格式：

```bash
# 使用 Inkscape 命令行导出
inkscape clipengine-logo.svg --export-type=png --export-width=800

# 或使用在线工具
# https://cloudconvert.com/svg-to-png
```

## 🎨 设计规范

### 颜色方案
```
主色调：
- 紫色渐变：#667eea → #764ba2
- 粉红渐变：#f093fb → #f5576c
- 蓝色：#4299e1
- 绿色：#48bb78
- 橙色：#ed8936

背景色：
- 深灰：#2d3748
- 中灰：#4a5568
- 浅灰：#e2e8f0
```

### 字体
- 标题：Arial / Microsoft YaHei (微软雅黑), Bold, 16-24px
- 正文：Arial / Microsoft YaHei, Regular, 11-14px
- 代码：Consolas / Courier New, 10-12px

### 阴影和圆角
- 圆角：4-8px
- 阴影：0 4px 6px rgba(0,0,0,0.1)

## 🛠️ 推荐工具

### 截图工具
- **Windows**: Snipping Tool, Snagit, ShareX
- **跨平台**: OBS Studio (录制)

### GIF 制作
- **ScreenToGif** - 轻量级录制工具
- **LICEcap** - 简单易用
- **Gifski** - 高质量GIF转换

### 图表制作
- **Python + Matplotlib** - 程序化生成性能图表
- **Excel / Google Sheets** - 快速制作图表
- **Chart.js** - 交互式图表

### 矢量图编辑
- **Inkscape** - 免费的SVG编辑器
- **Adobe Illustrator** - 专业矢量图工具
- **Figma** - 在线设计工具

### 图片编辑
- **GIMP** - 免费的位图编辑器
- **Photoshop** - 专业图像处理
- **Paint.NET** - 轻量级编辑器

## 📝 占位图生成

如果需要快速生成占位图进行测试，可以使用：

### 在线服务
- https://placeholder.com/
- https://via.placeholder.com/800x600.png?text=ClipEngine

### 命令行生成
```bash
# 使用 ImageMagick 生成占位图
convert -size 800x600 xc:gray -pointsize 48 -fill white -gravity center \
  -annotate +0+0 "ClipEngine Placeholder" placeholder.png
```

## 🔄 更新流程

1. 设计/截取新图片
2. 按照命名规范保存到本目录
3. 优化图片大小（PNG: TinyPNG, JPG: JPEGmini）
4. 更新本 README 中的状态
5. 验证文档中的引用路径

## 📐 尺寸参考

| 用途 | 推荐尺寸 | 格式 |
|------|---------|------|
| Logo | 400x120 px | SVG/PNG |
| 架构图 | 800-900x600 px | SVG/PNG |
| 演示GIF | 700x400 px | GIF |
| 截图 | 原始分辨率 | PNG |
| 图表 | 650x400 px | PNG/SVG |
| 案例缩略图 | 250x180 px | JPG/PNG |

## 🎯 下一步行动

1. **运行 Sample_sdk_demo 截取预览窗口**
   ```bash
   cd build/Debug/samples
   ./Sample_sdk_demo.exe
   # 截取运行中的窗口
   ```

2. **生成性能对比图**
   - 编写基准测试脚本
   - 收集性能数据
   - 用 Python/Excel 生成图表

3. **录制演示 GIF**
   - 准备演示场景
   - 使用 ScreenToGif 录制
   - 优化 GIF 大小（<5MB）

4. **设计应用案例插图**
   - 可以使用实际项目截图
   - 或者用 Figma 设计概念图

---

**更新日期**: 2025-11-03
**维护者**: ClipEngine Team
