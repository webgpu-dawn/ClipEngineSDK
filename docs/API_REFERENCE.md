# ClipEngine SDK API 参考文档

**Version**: 1.0.0
**Last Updated**: 2025-11-03
**Status**: Design Specification

---

## 📑 目录

- [1. 概述](#1-概述)
- [2. Project Management 项目管理](#2-project-management-项目管理)
- [3. Asset Management 资源管理](#3-asset-management-资源管理)
- [4. Timeline System 时间轴系统](#4-timeline-system-时间轴系统)
- [5. Animation System 动画系统](#5-animation-system-动画系统)
- [6. Color Management 色彩管理](#6-color-management-色彩管理)
- [7. Cache System 缓存系统](#7-cache-system-缓存系统)
- [8. 扩展图层类型](#8-扩展图层类型)
- [9. 扩展效果类型](#9-扩展效果类型)
- [10. Transform System 变换系统](#10-transform-system-变换系统)

---

## 1. 概述

本文档定义了 ClipEngine SDK 所有待实现模块的 API 接口设计。这些接口遵循以下原则：

- **一致性**: 与现有 API 风格保持一致
- **易用性**: 简洁直观的接口设计
- **性能**: 考虑性能优化的接口设计
- **扩展性**: 支持未来功能扩展

### 命名约定

- **类名**: PascalCase (`ProjectManager`, `AssetLibrary`)
- **函数名**: camelCase (`createProject()`, `loadAsset()`)
- **成员变量**: camelCase_ with trailing underscore (`projectPath_`, `assetMap_`)
- **常量/枚举**: UPPER_SNAKE_CASE or PascalCase

---

## 2. Project Management 项目管理

### 2.1 Project 类

项目是所有资源和合成的顶层容器。

```cpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Composition.h"
#include "AssetManager.h"

/**
 * @brief Project 类管理整个项目的生命周期
 *
 * 一个 Project 包含:
 * - 多个 Composition (合成)
 * - AssetLibrary (资源库)
 * - 项目设置 (分辨率、帧率、时长)
 * - 元数据 (作者、创建时间、版本)
 *
 * 文件格式: JSON (.ceproj)
 *
 * Example:
 * @code
 * // 创建新项目
 * ProjectConfig config = {
 *     .name = "My Video Project",
 *     .width = 1920,
 *     .height = 1080,
 *     .fps = 60,
 *     .duration = 120.0f  // 2 minutes
 * };
 *
 * auto project = Project::create(config);
 *
 * // 添加合成
 * auto mainComp = project->createComposition("Main", 1920, 1080, 60);
 *
 * // 保存项目
 * project->save("project.ceproj");
 *
 * // 加载项目
 * auto loadedProject = Project::load("project.ceproj");
 * @endcode
 */

struct ProjectConfig {
    std::string name = "Untitled Project";
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 60;
    float duration = 60.0f;  // seconds

    // 元数据
    std::string author = "";
    std::string description = "";
    std::string version = "1.0";
};

struct ProjectMetadata {
    std::string projectId;         // UUID
    std::string createdAt;         // ISO 8601 timestamp
    std::string modifiedAt;
    std::string author;
    std::string description;
    std::string version;
    std::vector<std::string> tags;
};

class Project {
public:
    /**
     * @brief 创建新项目
     */
    static std::unique_ptr<Project> create(const ProjectConfig& config);

    /**
     * @brief 从文件加载项目
     */
    static std::unique_ptr<Project> load(const std::string& path);

    /**
     * @brief 保存项目到文件
     * @param path 项目文件路径 (.ceproj)
     * @param options 保存选项
     * @return true if saved successfully
     */
    struct SaveOptions {
        bool compressAssets = false;     // 是否压缩资源
        bool embedAssets = false;        // 是否嵌入资源（否则使用相对路径）
        bool createBackup = true;        // 是否创建备份
        int compressionLevel = 6;        // 0-9
    };

    bool save(const std::string& path, const SaveOptions& options = {});

    /**
     * @brief 另存为
     */
    bool saveAs(const std::string& newPath, const SaveOptions& options = {});

    /**
     * @brief 关闭项目
     */
    void close();

    // ========== Composition Management ==========

    /**
     * @brief 创建新合成
     */
    Composition* createComposition(const std::string& name,
                                   uint32_t width,
                                   uint32_t height,
                                   uint32_t fps);

    /**
     * @brief 删除合成
     */
    bool deleteComposition(const std::string& name);

    /**
     * @brief 获取合成
     */
    Composition* getComposition(const std::string& name);
    Composition* getComposition(size_t index);

    /**
     * @brief 获取所有合成
     */
    std::vector<Composition*> getCompositions();

    /**
     * @brief 获取合成数量
     */
    size_t getCompositionCount() const;

    /**
     * @brief 设置主合成（用于渲染输出）
     */
    void setMainComposition(const std::string& name);
    Composition* getMainComposition();

    // ========== Asset Management ==========

    /**
     * @brief 获取资源管理器
     */
    AssetManager& getAssetManager();
    const AssetManager& getAssetManager() const;

    // ========== Project Settings ==========

    /**
     * @brief 获取/设置项目配置
     */
    const ProjectConfig& getConfig() const;
    void setConfig(const ProjectConfig& config);

    /**
     * @brief 获取/设置项目元数据
     */
    const ProjectMetadata& getMetadata() const;
    void setMetadata(const ProjectMetadata& metadata);

    /**
     * @brief 获取项目路径
     */
    const std::string& getProjectPath() const;

    /**
     * @brief 检查项目是否被修改
     */
    bool isDirty() const;

    /**
     * @brief 标记项目已保存
     */
    void markClean();

    // ========== Version Control (未来) ==========

    /**
     * @brief 创建项目快照
     */
    std::string createSnapshot(const std::string& description);

    /**
     * @brief 恢复到快照
     */
    bool restoreSnapshot(const std::string& snapshotId);

    /**
     * @brief 获取快照列表
     */
    std::vector<std::string> listSnapshots() const;

    // ========== Callbacks ==========

    /**
     * @brief 设置项目修改回调
     */
    void setDirtyCallback(std::function<void()> callback);

    /**
     * @brief 设置合成添加/删除回调
     */
    void setCompositionChangeCallback(
        std::function<void(const std::string& name, bool added)> callback);

private:
    Project() = default;

    std::string projectPath_;
    ProjectConfig config_;
    ProjectMetadata metadata_;
    bool dirty_ = false;

    std::vector<std::unique_ptr<Composition>> compositions_;
    Composition* mainComposition_ = nullptr;

    std::unique_ptr<AssetManager> assetManager_;

    // Callbacks
    std::function<void()> dirtyCallback_;
    std::function<void(const std::string&, bool)> compositionChangeCallback_;

    // Helpers
    bool loadFromJson(const std::string& path);
    bool saveToJson(const std::string& path, const SaveOptions& options);
    void markDirty();
};

/**
 * @brief Composition 类表示一个合成 (类似 AE 的 Composition)
 */
class Composition {
public:
    Composition(const std::string& name,
                uint32_t width,
                uint32_t height,
                uint32_t fps);

    // ========== Basic Properties ==========

    const std::string& getName() const;
    void setName(const std::string& name);

    uint32_t getWidth() const;
    uint32_t getHeight() const;
    void setResolution(uint32_t width, uint32_t height);

    uint32_t getFPS() const;
    void setFPS(uint32_t fps);

    float getDuration() const;
    void setDuration(float seconds);

    // ========== Layer Management ==========

    /**
     * @brief 添加图层
     */
    size_t addLayer(std::unique_ptr<CompositionLayer> layer);

    /**
     * @brief 删除图层
     */
    bool removeLayer(size_t index);

    /**
     * @brief 获取图层
     */
    CompositionLayer* getLayer(size_t index);
    CompositionLayer* getLayerByName(const std::string& name);

    /**
     * @brief 图层排序
     */
    void moveLayer(size_t fromIndex, size_t toIndex);
    void setLayerOrder(size_t index, int zOrder);

    size_t getLayerCount() const;

    // ========== Rendering ==========

    /**
     * @brief 渲染指定时间的帧
     */
    wgpu::TextureView render(float time, wgpu::Device device);

    /**
     * @brief 设置当前时间
     */
    void setCurrentTime(float time);
    float getCurrentTime() const;

    // ========== Background ==========

    struct BackgroundSettings {
        enum class Type { SolidColor, Gradient, Checkerboard };
        Type type = Type::SolidColor;
        wgpu::Color color1 = {0.0, 0.0, 0.0, 1.0};
        wgpu::Color color2 = {0.2, 0.2, 0.2, 1.0};
    };

    void setBackgroundSettings(const BackgroundSettings& settings);
    const BackgroundSettings& getBackgroundSettings() const;

private:
    std::string name_;
    uint32_t width_;
    uint32_t height_;
    uint32_t fps_;
    float duration_;
    float currentTime_ = 0.0f;

    std::vector<std::unique_ptr<CompositionLayer>> layers_;
    BackgroundSettings backgroundSettings_;

    std::unique_ptr<CompositionEngine> engine_;
};
```

---

## 3. Asset Management 资源管理

### 3.1 AssetManager 类

```cpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

/**
 * @brief Asset 基类
 */
enum class AssetType {
    Video,
    Image,
    Audio,
    Font,
    Effect,
    Lut,
    Unknown
};

enum class AssetStatus {
    Available,    // 资源可用
    Missing,      // 资源丢失
    Loading,      // 正在加载
    Error         // 加载错误
};

struct AssetMetadata {
    std::string assetId;          // UUID
    std::string name;             // 显示名称
    std::string originalPath;     // 原始文件路径
    AssetType type;               // 资源类型
    AssetStatus status;           // 资源状态
    uint64_t fileSize;            // 文件大小 (bytes)
    std::string createdAt;        // 导入时间
    std::string modifiedAt;       // 修改时间

    // 类型特定元数据
    std::unordered_map<std::string, std::string> metadata;  // width, height, duration, codec, etc.

    // 标签和分类
    std::vector<std::string> tags;
    std::string category;

    // 引用计数
    int referenceCount = 0;       // 被多少个图层引用

    // 缩略图
    std::string thumbnailPath;    // 缩略图路径
};

class Asset {
public:
    virtual ~Asset() = default;

    const std::string& getAssetId() const { return metadata_.assetId; }
    const std::string& getName() const { return metadata_.name; }
    AssetType getType() const { return metadata_.type; }
    AssetStatus getStatus() const { return metadata_.status; }
    const AssetMetadata& getMetadata() const { return metadata_; }

    void setName(const std::string& name) { metadata_.name = name; }
    void setTags(const std::vector<std::string>& tags) { metadata_.tags = tags; }
    void setCategory(const std::string& category) { metadata_.category = category; }

    void incrementReference() { metadata_.referenceCount++; }
    void decrementReference() { metadata_.referenceCount--; }
    int getReferenceCount() const { return metadata_.referenceCount; }

protected:
    AssetMetadata metadata_;
};

/**
 * @brief 视频资源
 */
class VideoAsset : public Asset {
public:
    VideoAsset(const std::string& path);

    uint32_t getWidth() const;
    uint32_t getHeight() const;
    float getDuration() const;
    uint32_t getFPS() const;
    std::string getCodec() const;
    uint64_t getBitrate() const;

private:
    // VideoSource 会在首次使用时创建
    bool analyzeVideo(const std::string& path);
};

/**
 * @brief 图片资源
 */
class ImageAsset : public Asset {
public:
    ImageAsset(const std::string& path);

    uint32_t getWidth() const;
    uint32_t getHeight() const;
    std::string getFormat() const;
    bool hasAlpha() const;

private:
    bool analyzeImage(const std::string& path);
};

/**
 * @brief 音频资源
 */
class AudioAsset : public Asset {
public:
    AudioAsset(const std::string& path);

    float getDuration() const;
    uint32_t getSampleRate() const;
    uint32_t getChannels() const;
    std::string getCodec() const;

private:
    bool analyzeAudio(const std::string& path);
};

/**
 * @brief AssetManager 管理项目中的所有资源
 */
class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;

    // ========== Import Assets ==========

    /**
     * @brief 导入资源
     * @param path 资源文件路径
     * @param options 导入选项
     * @return Asset ID (UUID)
     */
    struct ImportOptions {
        bool generateThumbnail = true;    // 生成缩略图
        bool copyToProject = false;       // 复制到项目目录
        bool analyzeMetadata = true;      // 分析元数据
        std::vector<std::string> tags;    // 初始标签
        std::string category;             // 分类
    };

    std::string importAsset(const std::string& path, const ImportOptions& options = {});

    /**
     * @brief 批量导入资源
     */
    std::vector<std::string> importAssets(const std::vector<std::string>& paths,
                                          const ImportOptions& options = {});

    /**
     * @brief 删除资源
     * @param assetId Asset UUID
     * @param deleteFile 是否删除源文件
     */
    bool removeAsset(const std::string& assetId, bool deleteFile = false);

    // ========== Query Assets ==========

    /**
     * @brief 根据 ID 获取资源
     */
    Asset* getAsset(const std::string& assetId);

    /**
     * @brief 根据路径获取资源
     */
    Asset* getAssetByPath(const std::string& path);

    /**
     * @brief 根据名称搜索资源
     */
    std::vector<Asset*> searchAssets(const std::string& query);

    /**
     * @brief 根据类型筛选资源
     */
    std::vector<Asset*> filterByType(AssetType type);

    /**
     * @brief 根据标签筛选资源
     */
    std::vector<Asset*> filterByTag(const std::string& tag);

    /**
     * @brief 获取所有资源
     */
    std::vector<Asset*> getAllAssets();

    /**
     * @brief 获取资源数量
     */
    size_t getAssetCount() const;

    // ========== Asset Operations ==========

    /**
     * @brief 重新链接丢失的资源
     */
    bool relinkAsset(const std::string& assetId, const std::string& newPath);

    /**
     * @brief 查找所有丢失的资源
     */
    std::vector<std::string> findMissingAssets();

    /**
     * @brief 替换资源
     * 保留所有引用，替换底层文件
     */
    bool replaceAsset(const std::string& assetId, const std::string& newPath);

    /**
     * @brief 复制资源
     */
    std::string duplicateAsset(const std::string& assetId);

    // ========== Thumbnail Management ==========

    /**
     * @brief 生成缩略图
     */
    bool generateThumbnail(const std::string& assetId, uint32_t width = 256, uint32_t height = 256);

    /**
     * @brief 获取缩略图路径
     */
    std::string getThumbnailPath(const std::string& assetId);

    // ========== Reference Tracking ==========

    /**
     * @brief 获取引用此资源的所有图层
     */
    std::vector<std::string> getReferences(const std::string& assetId);

    /**
     * @brief 查找未使用的资源
     */
    std::vector<std::string> findUnusedAssets();

    /**
     * @brief 清理未使用的资源
     */
    size_t cleanupUnusedAssets();

    // ========== Serialization ==========

    /**
     * @brief 导出资源库为 JSON
     */
    bool exportToJson(const std::string& path);

    /**
     * @brief 从 JSON 导入资源库
     */
    bool importFromJson(const std::string& path);

    // ========== Callbacks ==========

    /**
     * @brief 设置资源变更回调
     */
    void setAssetChangeCallback(
        std::function<void(const std::string& assetId, bool added)> callback);

private:
    std::unordered_map<std::string, std::unique_ptr<Asset>> assets_;
    std::string projectPath_;

    std::function<void(const std::string&, bool)> assetChangeCallback_;

    // Helpers
    std::string generateAssetId();
    AssetType detectAssetType(const std::string& path);
    std::unique_ptr<Asset> createAsset(const std::string& path, AssetType type);
};
```

---

## 4. Timeline System 时间轴系统

### 4.1 Timeline 类

```cpp
#pragma once

#include <vector>
#include <memory>
#include <functional>

/**
 * @brief 时间轴系统 - 管理项目的时间维度
 *
 * Timeline 负责:
 * - 时间码管理 (Timecode)
 * - 图层时间区间 (In/Out points)
 * - 播放控制 (Play/Pause/Stop)
 * - 时间标记 (Markers)
 * - 循环区域 (Work Area)
 *
 * Example:
 * @code
 * Timeline timeline;
 * timeline.setDuration(120.0f);  // 2 minutes
 * timeline.setFPS(60);
 *
 * // 播放控制
 * timeline.play();
 * timeline.pause();
 * timeline.stop();
 * timeline.seekTo(30.0f);
 *
 * // 标记
 * timeline.addMarker(10.5f, "Scene 1", MarkerColor::Blue);
 * timeline.addMarker(45.2f, "Transition");
 *
 * // 循环区域
 * timeline.setWorkArea(10.0f, 30.0f);
 * timeline.setLoopEnabled(true);
 * @endcode
 */

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

enum class MarkerColor {
    Red,
    Green,
    Blue,
    Yellow,
    Purple,
    Cyan,
    White
};

struct Marker {
    std::string id;
    float time;                    // seconds
    std::string label;
    std::string comment;
    MarkerColor color = MarkerColor::Red;
    int64_t frameNumber;          // 对应的帧号
};

struct WorkArea {
    float startTime = 0.0f;
    float endTime = 0.0f;
    bool enabled = false;
};

class Timeline {
public:
    Timeline();
    ~Timeline() = default;

    // ========== Time Management ==========

    /**
     * @brief 设置时间轴总时长
     */
    void setDuration(float seconds);
    float getDuration() const;

    /**
     * @brief 设置帧率
     */
    void setFPS(uint32_t fps);
    uint32_t getFPS() const;

    /**
     * @brief 获取总帧数
     */
    int64_t getTotalFrames() const;

    // ========== Playback Control ==========

    /**
     * @brief 播放
     */
    void play();

    /**
     * @brief 暂停
     */
    void pause();

    /**
     * @brief 停止 (回到开始)
     */
    void stop();

    /**
     * @brief 切换播放/暂停
     */
    void togglePlayPause();

    /**
     * @brief 获取播放状态
     */
    PlaybackState getPlaybackState() const;

    // ========== Seek Operations ==========

    /**
     * @brief 跳转到指定时间
     */
    void seekTo(float seconds);

    /**
     * @brief 跳转到指定帧
     */
    void seekToFrame(int64_t frame);

    /**
     * @brief 跳转到开始
     */
    void seekToStart();

    /**
     * @brief 跳转到结束
     */
    void seekToEnd();

    /**
     * @brief 下一帧
     */
    void stepForward();

    /**
     * @brief 上一帧
     */
    void stepBackward();

    /**
     * @brief 获取当前时间
     */
    float getCurrentTime() const;

    /**
     * @brief 获取当前帧号
     */
    int64_t getCurrentFrame() const;

    // ========== Markers ==========

    /**
     * @brief 添加标记
     */
    std::string addMarker(float time,
                         const std::string& label = "",
                         MarkerColor color = MarkerColor::Red);

    /**
     * @brief 删除标记
     */
    bool removeMarker(const std::string& markerId);

    /**
     * @brief 获取标记
     */
    Marker* getMarker(const std::string& markerId);

    /**
     * @brief 获取所有标记
     */
    std::vector<Marker*> getAllMarkers();

    /**
     * @brief 跳转到下一个标记
     */
    void goToNextMarker();

    /**
     * @brief 跳转到上一个标记
     */
    void goToPreviousMarker();

    // ========== Work Area (Loop Region) ==========

    /**
     * @brief 设置工作区域
     */
    void setWorkArea(float startTime, float endTime);

    /**
     * @brief 清除工作区域
     */
    void clearWorkArea();

    /**
     * @brief 获取工作区域
     */
    const WorkArea& getWorkArea() const;

    /**
     * @brief 启用/禁用循环播放
     */
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const;

    // ========== Update ==========

    /**
     * @brief 更新时间轴 (每帧调用)
     * @param deltaTime Time since last frame (seconds)
     */
    void update(float deltaTime);

    // ========== Playback Speed ==========

    /**
     * @brief 设置播放速度
     * @param speed 1.0 = normal, 0.5 = half speed, 2.0 = double speed
     */
    void setPlaybackSpeed(float speed);
    float getPlaybackSpeed() const;

    // ========== Time Code ==========

    /**
     * @brief 获取时间码字符串 (HH:MM:SS:FF)
     */
    std::string getTimecode(float time) const;
    std::string getCurrentTimecode() const;

    /**
     * @brief 从时间码字符串解析时间
     */
    float parseTimecode(const std::string& timecode) const;

    // ========== Callbacks ==========

    /**
     * @brief 设置时间变更回调
     */
    void setTimeChangeCallback(std::function<void(float time)> callback);

    /**
     * @brief 设置播放状态变更回调
     */
    void setPlaybackStateCallback(std::function<void(PlaybackState state)> callback);

private:
    float duration_ = 60.0f;
    uint32_t fps_ = 60;
    float currentTime_ = 0.0f;
    PlaybackState playbackState_ = PlaybackState::Stopped;
    float playbackSpeed_ = 1.0f;
    bool loopEnabled_ = false;

    WorkArea workArea_;
    std::vector<Marker> markers_;

    std::function<void(float)> timeChangeCallback_;
    std::function<void(PlaybackState)> playbackStateCallback_;

    void notifyTimeChange();
    void notifyPlaybackStateChange();
};
```

---

## 5. Animation System 动画系统

### 5.1 Keyframe Animation

```cpp
#pragma once

#include <vector>
#include <functional>
#include <variant>
#include <glm/glm.hpp>

/**
 * @brief 缓动函数类型
 */
enum class EasingFunction {
    Linear,
    EaseIn, EaseOut, EaseInOut,
    QuadIn, QuadOut, QuadInOut,
    CubicIn, CubicOut, CubicInOut,
    QuartIn, QuartOut, QuartInOut,
    QuintIn, QuintOut, QuintInOut,
    SineIn, SineOut, SineInOut,
    ExpoIn, ExpoOut, ExpoInOut,
    CircIn, CircOut, CircInOut,
    BackIn, BackOut, BackInOut,
    ElasticIn, ElasticOut, ElasticInOut,
    BounceIn, BounceOut, BounceInOut,
    Custom  // 使用贝塞尔曲线
};

/**
 * @brief 关键帧插值类型
 */
enum class InterpolationType {
    Constant,  // 阶跃
    Linear,    // 线性插值
    Bezier,    // 贝塞尔插值
    Hold       // 保持 (不插值)
};

/**
 * @brief 关键帧数据类型
 */
using KeyframeValue = std::variant<
    float,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    int,
    bool
>;

/**
 * @brief 关键帧
 */
template<typename T>
struct Keyframe {
    float time;                       // 时间 (seconds)
    T value;                          // 值
    InterpolationType interpolation;  // 插值类型
    EasingFunction easing;            // 缓动函数

    // 贝塞尔控制点 (用于 Bezier interpolation)
    glm::vec2 inTangent = {0.0f, 0.0f};
    glm::vec2 outTangent = {0.0f, 0.0f};
};

/**
 * @brief 可动画属性
 *
 * Example:
 * @code
 * // 创建位置动画
 * AnimatableProperty<glm::vec2> position;
 * position.addKeyframe(0.0f, {0.0f, 0.0f}, EasingFunction::Linear);
 * position.addKeyframe(2.0f, {100.0f, 200.0f}, EasingFunction::EaseInOut);
 * position.addKeyframe(4.0f, {0.0f, 0.0f}, EasingFunction::EaseOut);
 *
 * // 获取插值后的值
 * auto value = position.evaluate(1.0f);  // t=1.0秒时的位置
 *
 * // 使用贝塞尔曲线
 * position.addKeyframe(6.0f, {200.0f, 100.0f},
 *     EasingFunction::Custom,
 *     InterpolationType::Bezier);
 * position.setInTangent(2, {0.5f, 0.0f});
 * position.setOutTangent(2, {0.5f, 1.0f});
 * @endcode
 */
template<typename T>
class AnimatableProperty {
public:
    AnimatableProperty() = default;
    AnimatableProperty(const T& initialValue) : currentValue_(initialValue) {}

    // ========== Keyframe Management ==========

    /**
     * @brief 添加关键帧
     */
    size_t addKeyframe(float time,
                      const T& value,
                      EasingFunction easing = EasingFunction::Linear,
                      InterpolationType interpolation = InterpolationType::Linear);

    /**
     * @brief 删除关键帧
     */
    bool removeKeyframe(size_t index);

    /**
     * @brief 删除指定时间的关键帧
     */
    bool removeKeyframeAtTime(float time, float tolerance = 0.001f);

    /**
     * @brief 清空所有关键帧
     */
    void clearKeyframes();

    /**
     * @brief 获取关键帧数量
     */
    size_t getKeyframeCount() const;

    /**
     * @brief 获取关键帧
     */
    Keyframe<T>* getKeyframe(size_t index);
    const Keyframe<T>* getKeyframe(size_t index) const;

    /**
     * @brief 查找指定时间附近的关键帧
     */
    int findKeyframeNear(float time, float tolerance = 0.001f) const;

    // ========== Evaluation ==========

    /**
     * @brief 计算指定时间的值 (插值)
     */
    T evaluate(float time) const;

    /**
     * @brief 获取当前值
     */
    const T& getValue() const { return currentValue_; }

    /**
     * @brief 设置当前值 (不添加关键帧)
     */
    void setValue(const T& value) { currentValue_ = value; }

    // ========== Bezier Tangent Control ==========

    /**
     * @brief 设置入切线
     */
    void setInTangent(size_t keyframeIndex, const glm::vec2& tangent);

    /**
     * @brief 设置出切线
     */
    void setOutTangent(size_t keyframeIndex, const glm::vec2& tangent);

    // ========== Animation Control ==========

    /**
     * @brief 检查是否有关键帧动画
     */
    bool isAnimated() const { return !keyframes_.empty(); }

    /**
     * @brief 获取动画时间范围
     */
    std::pair<float, float> getTimeRange() const;

    /**
     * @brief 时间缩放
     */
    void scaleTime(float scale);

    /**
     * @brief 时间偏移
     */
    void offsetTime(float offset);

private:
    T currentValue_;
    std::vector<Keyframe<T>> keyframes_;

    // Interpolation helpers
    T interpolateLinear(const Keyframe<T>& k1, const Keyframe<T>& k2, float t) const;
    T interpolateBezier(const Keyframe<T>& k1, const Keyframe<T>& k2, float t) const;
    float applyEasing(float t, EasingFunction easing) const;
};

/**
 * @brief 表达式引擎 (未来功能)
 *
 * 类似 After Effects 的表达式系统
 */
class Expression {
public:
    /**
     * @brief 设置表达式代码 (JavaScript-like syntax)
     */
    void setCode(const std::string& code);

    /**
     * @brief 计算表达式值
     */
    float evaluate(float time, const std::unordered_map<std::string, float>& variables);

    /**
     * @brief 检查表达式是否有效
     */
    bool isValid() const;

    /**
     * @brief 获取错误信息
     */
    const std::string& getError() const;

private:
    std::string code_;
    bool valid_ = false;
    std::string error_;
};

// 预定义的属性类型别名
using AnimatableFloat = AnimatableProperty<float>;
using AnimatableVec2 = AnimatableProperty<glm::vec2>;
using AnimatableVec3 = AnimatableProperty<glm::vec3>;
using AnimatableVec4 = AnimatableProperty<glm::vec4>;
using AnimatableColor = AnimatableProperty<glm::vec4>;
```

---

## 6. Color Management 色彩管理

### 6.1 ColorSpace 类

```cpp
#pragma once

#include <string>
#include <memory>
#include <vector>

/**
 * @brief 色彩空间定义
 */
enum class ColorSpace {
    // RGB 色彩空间
    sRGB,
    LinearRec709,
    Rec709,
    Rec2020,
    LinearRec2020,
    DCIP3,
    ACES_AP0,
    ACES_AP1,
    ACEScc,
    ACEScct,

    // 其他
    Custom
};

/**
 * @brief 传输函数 (Transfer Function / Gamma)
 */
enum class TransferFunction {
    Linear,         // 线性 (gamma = 1.0)
    sRGB,          // sRGB gamma (2.2 近似)
    Rec709,        // ITU-R BT.709
    Gamma22,       // gamma = 2.2
    Gamma24,       // gamma = 2.4
    PQ,            // Perceptual Quantizer (HDR)
    HLG,           // Hybrid Log-Gamma (HDR)
    Log,           // Logarithmic
    Custom
};

/**
 * @brief LUT (Look-Up Table) 类型
 */
enum class LUTFormat {
    LUT_1D,    // 1D LUT
    LUT_3D,    // 3D LUT
    CUBE,      // .cube 格式
    CSP,       // .csp 格式
    ICC        // ICC Profile
};

/**
 * @brief 色彩管理配置
 */
struct ColorConfig {
    ColorSpace workingSpace = ColorSpace::LinearRec709;  // 工作色彩空间
    ColorSpace outputSpace = ColorSpace::sRGB;           // 输出色彩空间
    bool useOCIO = false;                                // 使用 OpenColorIO
    std::string ocioConfigPath;                          // OCIO 配置文件路径
    std::string ocioLook;                                // OCIO Look
};

/**
 * @brief LUT (Look-Up Table)
 *
 * Example:
 * @code
 * // 加载 3D LUT
 * auto lut = ColorLUT::load("cinematic_look.cube");
 * if (lut) {
 *     lut->setIntensity(0.8f);  // 80% 强度
 *
 *     // 应用到合成
 *     composition->setOutputLUT(lut);
 * }
 * @endcode
 */
class ColorLUT {
public:
    /**
     * @brief 从文件加载 LUT
     */
    static std::unique_ptr<ColorLUT> load(const std::string& path);

    /**
     * @brief 创建 1D LUT
     */
    static std::unique_ptr<ColorLUT> create1D(size_t size);

    /**
     * @brief 创建 3D LUT
     */
    static std::unique_ptr<ColorLUT> create3D(size_t size);

    /**
     * @brief 获取 LUT 类型
     */
    LUTFormat getFormat() const;

    /**
     * @brief 获取 LUT 大小
     */
    size_t getSize() const;

    /**
     * @brief 应用 LUT
     * @param input RGB color [0, 1]
     * @return Transformed RGB color
     */
    glm::vec3 apply(const glm::vec3& input) const;

    /**
     * @brief 设置 LUT 强度
     * @param intensity 0.0 = no effect, 1.0 = full effect
     */
    void setIntensity(float intensity);
    float getIntensity() const;

    /**
     * @brief 上传到 GPU (创建 3D 纹理)
     */
    wgpu::Texture createGPUTexture(wgpu::Device device);

private:
    ColorLUT() = default;

    LUTFormat format_;
    size_t size_;
    float intensity_ = 1.0f;
    std::vector<glm::vec3> data_;  // LUT 数据

    bool loadCube(const std::string& path);
    bool loadCSP(const std::string& path);
};

/**
 * @brief 色彩管理器
 *
 * Example:
 * @code
 * ColorManager colorMgr;
 *
 * // 配置工作流
 * ColorConfig config = {
 *     .workingSpace = ColorSpace::LinearRec709,
 *     .outputSpace = ColorSpace::sRGB
 * };
 * colorMgr.setConfig(config);
 *
 * // 色彩空间转换
 * glm::vec3 srgb = {0.5f, 0.2f, 0.8f};
 * glm::vec3 linear = colorMgr.transform(srgb,
 *     ColorSpace::sRGB,
 *     ColorSpace::LinearRec709);
 *
 * // 使用 OCIO
 * colorMgr.loadOCIOConfig("aces_1.2/config.ocio");
 * glm::vec3 output = colorMgr.applyOCIOTransform(input,
 *     "ACES - ACEScg",
 *     "Output - sRGB");
 * @endcode
 */
class ColorManager {
public:
    ColorManager() = default;
    ~ColorManager() = default;

    // ========== Configuration ==========

    /**
     * @brief 设置色彩配置
     */
    void setConfig(const ColorConfig& config);
    const ColorConfig& getConfig() const;

    // ========== Color Space Transforms ==========

    /**
     * @brief 色彩空间转换
     */
    glm::vec3 transform(const glm::vec3& color,
                        ColorSpace fromSpace,
                        ColorSpace toSpace);

    /**
     * @brief 应用传输函数
     */
    glm::vec3 applyTransferFunction(const glm::vec3& color,
                                    TransferFunction func);

    /**
     * @brief 反向传输函数
     */
    glm::vec3 inverseTransferFunction(const glm::vec3& color,
                                      TransferFunction func);

    // ========== OpenColorIO ==========

    /**
     * @brief 加载 OCIO 配置文件
     */
    bool loadOCIOConfig(const std::string& path);

    /**
     * @brief 应用 OCIO 变换
     */
    glm::vec3 applyOCIOTransform(const glm::vec3& color,
                                  const std::string& srcColorSpace,
                                  const std::string& dstColorSpace);

    /**
     * @brief 获取 OCIO 可用的色彩空间列表
     */
    std::vector<std::string> getOCIOColorSpaces() const;

    /**
     * @brief 获取 OCIO 可用的 Look 列表
     */
    std::vector<std::string> getOCIOLooks() const;

    // ========== LUT Management ==========

    /**
     * @brief 添加 LUT
     */
    void addLUT(const std::string& name, std::shared_ptr<ColorLUT> lut);

    /**
     * @brief 获取 LUT
     */
    std::shared_ptr<ColorLUT> getLUT(const std::string& name);

    /**
     * @brief 移除 LUT
     */
    void removeLUT(const std::string& name);

    // ========== GPU Shader Generation ==========

    /**
     * @brief 生成色彩管理 shader 代码
     * 用于在 GPU 上执行色彩转换
     */
    std::string generateColorManagementShader(ColorSpace fromSpace,
                                               ColorSpace toSpace);

private:
    ColorConfig config_;
    std::unordered_map<std::string, std::shared_ptr<ColorLUT>> luts_;

    // OCIO context (opaque pointer)
    void* ocioConfig_ = nullptr;

    // Transform matrices
    glm::mat3 getTransformMatrix(ColorSpace fromSpace, ColorSpace toSpace);
};
```

---

## 7. Cache System 缓存系统

### 7.1 CacheManager 类

```cpp
#pragma once

#include <string>
#include <memory>
#include <functional>
#include <optional>

/**
 * @brief 三层缓存系统
 *
 * L1: RAM (2GB) - 最近渲染的帧
 * L2: SSD (50GB) - 预渲染的帧
 * L3: Network (Unlimited) - 分布式渲染农场
 *
 * Example:
 * @code
 * CacheManager cache;
 *
 * CacheConfig config = {
 *     .l1MaxSize = 2 * 1024 * 1024 * 1024,  // 2 GB
 *     .l2MaxSize = 50 * 1024 * 1024 * 1024, // 50 GB
 *     .l2Path = "D:/ClipEngineCache",
 *     .enableL3 = false
 * };
 * cache.initialize(config);
 *
 * // 缓存查询
 * std::string key = "comp_main_layer0_t1.523_1920x1080";
 * auto frame = cache.get(key);
 *
 * if (!frame) {
 *     // 缓存未命中，渲染帧
 *     frame = renderFrame(...);
 *     cache.put(key, frame);
 * }
 * @endcode
 */

struct CacheConfig {
    // L1 Cache (RAM)
    uint64_t l1MaxSize = 2ULL * 1024 * 1024 * 1024;  // 2 GB
    bool enableL1 = true;

    // L2 Cache (Disk)
    uint64_t l2MaxSize = 50ULL * 1024 * 1024 * 1024;  // 50 GB
    std::string l2Path = "./cache";
    bool enableL2 = true;
    bool compressL2 = true;  // 压缩存储

    // L3 Cache (Network)
    bool enableL3 = false;
    std::string l3ServerUrl;
    std::string l3AuthToken;

    // Eviction policy
    enum class EvictionPolicy {
        LRU,    // Least Recently Used
        LFU,    // Least Frequently Used
        FIFO    // First In First Out
    };
    EvictionPolicy l1EvictionPolicy = EvictionPolicy::LRU;
    EvictionPolicy l2EvictionPolicy = EvictionPolicy::LRU;
};

struct CacheEntry {
    std::string key;
    std::vector<uint8_t> data;
    uint64_t size;
    uint64_t accessCount;
    std::chrono::system_clock::time_point lastAccessTime;
    std::chrono::system_clock::time_point createdTime;
};

struct CacheStats {
    // Hit rates
    uint64_t l1Hits = 0;
    uint64_t l2Hits = 0;
    uint64_t l3Hits = 0;
    uint64_t misses = 0;

    // Sizes
    uint64_t l1CurrentSize = 0;
    uint64_t l2CurrentSize = 0;
    uint64_t l3CurrentSize = 0;

    // Counts
    uint64_t l1EntryCount = 0;
    uint64_t l2EntryCount = 0;
    uint64_t l3EntryCount = 0;

    // Computed metrics
    float getL1HitRate() const {
        uint64_t total = l1Hits + l2Hits + l3Hits + misses;
        return total > 0 ? static_cast<float>(l1Hits) / total : 0.0f;
    }

    float getTotalHitRate() const {
        uint64_t total = l1Hits + l2Hits + l3Hits + misses;
        uint64_t hits = l1Hits + l2Hits + l3Hits;
        return total > 0 ? static_cast<float>(hits) / total : 0.0f;
    }
};

class CacheManager {
public:
    CacheManager() = default;
    ~CacheManager();

    // ========== Initialization ==========

    /**
     * @brief 初始化缓存系统
     */
    bool initialize(const CacheConfig& config);

    /**
     * @brief 关闭缓存系统
     */
    void shutdown();

    // ========== Cache Operations ==========

    /**
     * @brief 查询缓存
     * @param key 缓存键
     * @return 缓存数据，如果未找到返回 nullopt
     */
    std::optional<std::vector<uint8_t>> get(const std::string& key);

    /**
     * @brief 存入缓存
     * @param key 缓存键
     * @param data 数据
     * @param forceL2 强制存入 L2 (用于预渲染)
     */
    bool put(const std::string& key, const std::vector<uint8_t>& data, bool forceL2 = false);

    /**
     * @brief 检查缓存是否存在
     */
    bool contains(const std::string& key);

    /**
     * @brief 删除缓存项
     */
    bool remove(const std::string& key);

    /**
     * @brief 清空所有缓存
     */
    void clear();

    /**
     * @brief 清空指定层级缓存
     */
    void clearL1();
    void clearL2();
    void clearL3();

    // ========== Cache Management ==========

    /**
     * @brief 预热缓存 (预渲染指定范围的帧)
     */
    void preheat(const std::vector<std::string>& keys,
                 std::function<std::vector<uint8_t>(const std::string&)> renderFunc);

    /**
     * @brief 手动触发缓存清理
     */
    void evict();

    /**
     * @brief 压缩 L2 缓存
     */
    void compactL2();

    // ========== Statistics ==========

    /**
     * @brief 获取缓存统计信息
     */
    const CacheStats& getStats() const;

    /**
     * @brief 重置统计信息
     */
    void resetStats();

    /**
     * @brief 打印缓存统计
     */
    void printStats() const;

    // ========== Configuration ==========

    /**
     * @brief 获取配置
     */
    const CacheConfig& getConfig() const;

    /**
     * @brief 更新配置 (部分参数需要重启)
     */
    void updateConfig(const CacheConfig& config);

private:
    CacheConfig config_;
    CacheStats stats_;

    // L1 Cache (RAM)
    std::unordered_map<std::string, CacheEntry> l1Cache_;
    std::mutex l1Mutex_;

    // L2 Cache (Disk)
    std::string l2CachePath_;
    std::unordered_map<std::string, std::string> l2Index_;  // key -> file path
    std::mutex l2Mutex_;

    // L3 Cache (Network) - future
    // Network client implementation

    // Helpers
    void evictL1();
    void evictL2();
    bool loadFromL2(const std::string& key, std::vector<uint8_t>& data);
    bool saveToL2(const std::string& key, const std::vector<uint8_t>& data);
};

/**
 * @brief 缓存键生成器
 */
class CacheKeyGenerator {
public:
    /**
     * @brief 生成渲染缓存键
     */
    static std::string generateRenderKey(
        const std::string& compositionId,
        const std::string& layerId,
        float time,
        uint32_t width,
        uint32_t height,
        const std::string& effectsHash = "");

    /**
     * @brief 生成效果缓存键
     */
    static std::string generateEffectKey(
        const std::string& effectName,
        const std::string& parametersHash);

    /**
     * @brief 计算数据哈希
     */
    static std::string hashData(const void* data, size_t size);

    /**
     * @brief 计算字符串哈希
     */
    static std::string hashString(const std::string& str);
};
```

---

## 8. 扩展图层类型

### 8.1 ImageLayer

```cpp
#pragma once

#include "core/CompositionLayer.h"
#include <string>

/**
 * @brief 图片图层
 *
 * 支持格式: PNG, JPG, TGA, BMP, etc. (通过 stb_image)
 */
class ImageLayer : public CompositionLayer {
public:
    ImageLayer();
    ~ImageLayer() override;

    /**
     * @brief 加载图片文件
     */
    bool loadImage(const std::string& path);

    /**
     * @brief 从内存加载图片
     */
    bool loadImageFromMemory(const uint8_t* data, size_t size);

    /**
     * @brief 从 Asset 加载
     */
    bool loadFromAsset(const std::string& assetId, AssetManager* assetMgr);

    /**
     * @brief 渲染图层
     */
    wgpu::TextureView render() override;

    /**
     * @brief 更新图层
     */
    void update(float deltaTime) override;

    /**
     * @brief 获取图片尺寸
     */
    uint32_t getImageWidth() const;
    uint32_t getImageHeight() const;

private:
    std::string imagePath_;
    wgpu::Texture texture_;
    wgpu::TextureView textureView_;
    uint32_t imageWidth_ = 0;
    uint32_t imageHeight_ = 0;

    bool createGPUTexture(const uint8_t* pixels,
                          uint32_t width,
                          uint32_t height,
                          int channels);
};
```

### 8.2 TextLayer

```cpp
#pragma once

#include "core/CompositionLayer.h"
#include <string>
#include <glm/glm.hpp>

/**
 * @brief 文本图层
 *
 * 支持:
 * - TrueType/OpenType 字体
 * - 文本样式 (粗体、斜体、下划线)
 * - 文本对齐
 * - 字符间距、行间距
 * - 渐变填充
 * - 描边
 */

enum class TextAlignment {
    Left,
    Center,
    Right,
    Justify
};

struct TextStyle {
    std::string fontFamily = "Arial";
    float fontSize = 48.0f;
    bool bold = false;
    bool italic = false;
    bool underline = false;

    glm::vec4 fillColor = {1.0f, 1.0f, 1.0f, 1.0f};
    bool enableStroke = false;
    glm::vec4 strokeColor = {0.0f, 0.0f, 0.0f, 1.0f};
    float strokeWidth = 2.0f;

    TextAlignment alignment = TextAlignment::Left;
    float letterSpacing = 0.0f;  // em units
    float lineHeight = 1.2f;     // relative to font size

    // Future: Gradient fill, shadow, etc.
};

class TextLayer : public CompositionLayer {
public:
    TextLayer();
    ~TextLayer() override;

    /**
     * @brief 设置文本内容
     */
    void setText(const std::string& text);
    const std::string& getText() const;

    /**
     * @brief 设置文本样式
     */
    void setStyle(const TextStyle& style);
    const TextStyle& getStyle() const;

    /**
     * @brief 加载字体文件
     */
    bool loadFont(const std::string& path);

    /**
     * @brief 渲染图层
     */
    wgpu::TextureView render() override;

    /**
     * @brief 更新图层
     */
    void update(float deltaTime) override;

    /**
     * @brief 获取文本边界框
     */
    struct TextBounds {
        float width;
        float height;
    };
    TextBounds calculateBounds() const;

private:
    std::string text_;
    TextStyle style_;

    wgpu::Texture renderTarget_;
    wgpu::TextureView renderTargetView_;

    // Font rendering context (e.g., FreeType)
    void* fontContext_ = nullptr;

    void rebuildTextTexture();
};
```

---

## 9. 扩展效果类型

### 9.1 Distortion Effects

```cpp
#pragma once

#include "effects/Filter.h"

/**
 * @brief 镜头扭曲效果
 */
class LensDistortionEffect : public Filter {
public:
    LensDistortionEffect();

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void apply(wgpu::RenderPassEncoder& pass,
               const std::vector<wgpu::TextureView>& inputs) override;
    void updateParameters() override;
    const std::string& getName() const override { return name_; }

    // Parameters
    void setDistortion(float amount);    // -1.0 to 1.0 (negative = pincushion, positive = barrel)
    void setChromaticAberration(float amount);  // 0.0 to 1.0
    void setCenter(const glm::vec2& center);    // 0.0 to 1.0 (normalized)

private:
    std::string name_ = "Lens Distortion";
    float distortion_ = 0.0f;
    float chromaticAberration_ = 0.0f;
    glm::vec2 center_ = {0.5f, 0.5f};
};

/**
 * @brief 涟漪效果
 */
class RippleEffect : public Filter {
public:
    RippleEffect();

    bool initialize(wgpu::Device device, wgpu::TextureFormat format) override;
    void apply(wgpu::RenderPassEncoder& pass,
               const std::vector<wgpu::TextureView>& inputs) override;
    void updateParameters() override;
    const std::string& getName() const override { return name_; }

    // Parameters
    void setAmplitude(float amplitude);  // Wave height
    void setFrequency(float frequency);  // Wave frequency
    void setPhase(float phase);          // Animation phase
    void setCenter(const glm::vec2& center);

private:
    std::string name_ = "Ripple";
    float amplitude_ = 10.0f;
    float frequency_ = 5.0f;
    float phase_ = 0.0f;
    glm::vec2 center_ = {0.5f, 0.5f};
};
```

---

## 10. Transform System 变换系统

### 10.1 完整的 Transform 类

```cpp
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Animation.h"

/**
 * @brief 完整的 2D/3D 变换系统
 *
 * 支持:
 * - Position (位置)
 * - Rotation (旋转)
 * - Scale (缩放)
 * - Anchor Point (锚点)
 * - Skew (倾斜)
 * - 3D Transforms (未来)
 */
class Transform {
public:
    Transform();

    // ========== 2D Transform Properties ==========

    AnimatableVec2 position = AnimatableVec2({0.0f, 0.0f});
    AnimatableFloat rotation = AnimatableFloat(0.0f);  // degrees
    AnimatableVec2 scale = AnimatableVec2({1.0f, 1.0f});
    AnimatableVec2 anchorPoint = AnimatableVec2({0.0f, 0.0f});  // pixels
    AnimatableVec2 skew = AnimatableVec2({0.0f, 0.0f});         // degrees
    AnimatableFloat opacity = AnimatableFloat(1.0f);             // 0.0 to 1.0

    // ========== 3D Transform Properties (未来) ==========

    AnimatableVec3 position3D = AnimatableVec3({0.0f, 0.0f, 0.0f});
    AnimatableVec3 rotation3D = AnimatableVec3({0.0f, 0.0f, 0.0f});  // euler angles
    AnimatableVec3 scale3D = AnimatableVec3({1.0f, 1.0f, 1.0f});

    // ========== Matrix Computation ==========

    /**
     * @brief 计算 2D 变换矩阵
     */
    glm::mat3 computeMatrix2D(float time) const;

    /**
     * @brief 计算 3D 变换矩阵
     */
    glm::mat4 computeMatrix3D(float time) const;

    /**
     * @brief 计算当前变换矩阵 (基于时间)
     */
    glm::mat3 getMatrix2D(float time) const { return computeMatrix2D(time); }
    glm::mat4 getMatrix3D(float time) const { return computeMatrix3D(time); }

    // ========== Parent/Child Hierarchy ==========

    /**
     * @brief 设置父变换
     */
    void setParent(Transform* parent);
    Transform* getParent() const { return parent_; }

    /**
     * @brief 计算世界空间矩阵 (包含父级变换)
     */
    glm::mat3 getWorldMatrix2D(float time) const;
    glm::mat4 getWorldMatrix3D(float time) const;

    // ========== Utility Functions ==========

    /**
     * @brief 重置所有属性
     */
    void reset();

    /**
     * @brief 检查是否有动画
     */
    bool isAnimated() const;

private:
    Transform* parent_ = nullptr;
};
```

---

## 总结

本文档定义了 ClipEngine SDK 待实现模块的完整 API 接口设计，包括：

1. ✅ **Project Management** - 项目和合成管理
2. ✅ **Asset Management** - 资源管理和引用追踪
3. ✅ **Timeline System** - 时间轴和播放控制
4. ✅ **Animation System** - 关键帧动画和缓动
5. ✅ **Color Management** - 色彩空间转换和 LUT
6. ✅ **Cache System** - 三层缓存系统
7. ✅ **Extended Layers** - Image/Text/Shape 图层
8. ✅ **Extended Effects** - 扭曲/风格化效果
9. ✅ **Transform System** - 完整的变换系统

这些接口设计遵循现有代码风格，与当前实现保持一致，并为未来扩展预留空间。

---

**下一步**:

1. 实现 Project Management 模块
2. 实现 Asset Management 模块
3. 实现 Animation System
4. 逐步完善其他模块

详见 [ARCHITECTURE.md](../ARCHITECTURE.md) 的实现规划部分。
