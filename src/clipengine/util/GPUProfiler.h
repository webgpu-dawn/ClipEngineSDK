#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <webgpu/webgpu_cpp.h>
#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <limits>

namespace ClipEngine {

/**
 * @brief GPU 性能分析器
 *
 * 用于测量和记录 GPU 操作的性能指标，包括：
 * - GPU 拷贝操作耗时
 * - 渲染 Pass 耗时
 * - 计算 Pass 耗时
 * - 帧时间统计
 */
class GPUProfiler {
public:
    struct ProfileResult {
        std::string name;           // 测量名称
        double cpu_time_ms;         // CPU 端耗时（毫秒）
        double gpu_time_ms;         // GPU 端耗时（毫秒，需要 timestamp query 支持）
        uint64_t call_count;        // 调用次数
        double avg_time_ms;         // 平均耗时
        double min_time_ms;         // 最小耗时
        double max_time_ms;         // 最大耗时
    };

    struct FrameStats {
        double frame_time_ms;       // 帧时间
        double render_time_ms;      // 渲染时间
        double copy_time_ms;        // 拷贝时间
        uint64_t frame_number;      // 帧编号
    };

    explicit GPUProfiler(const wgpu::Device& device);
    ~GPUProfiler();

    /**
     * @brief 开始一个性能测量区域
     * @param name 测量区域名称
     */
    void beginEvent(const std::string& name);

    /**
     * @brief 结束当前性能测量区域
     */
    void endEvent();

    /**
     * @brief 标记帧开始
     */
    void beginFrame();

    /**
     * @brief 标记帧结束
     */
    void endFrame();

    /**
     * @brief 开始 GPU 计时查询（需要 timestamp-query 特性）
     * @param encoder 命令编码器
     * @param name 查询名称
     */
    void beginGPUQuery(wgpu::CommandEncoder encoder, const std::string& name);

    /**
     * @brief 结束 GPU 计时查询
     * @param encoder 命令编码器
     */
    void endGPUQuery(wgpu::CommandEncoder encoder);

    /**
     * @brief 获取所有性能结果
     * @return 性能结果列表
     */
    std::vector<ProfileResult> getResults() const;

    /**
     * @brief 获取最近 N 帧的统计信息
     * @param frame_count 帧数量
     * @return 帧统计列表
     */
    std::vector<FrameStats> getFrameStats(size_t frame_count = 60) const;

    /**
     * @brief 打印性能报告到控制台
     */
    void printReport() const;

    /**
     * @brief 重置所有统计数据
     */
    void reset();

    /**
     * @brief 启用/禁用性能分析
     * @param enable 是否启用
     */
    void setEnabled(bool enable) { m_enabled = enable; }

    /**
     * @brief 检查是否启用了性能分析
     */
    bool isEnabled() const { return m_enabled; }

private:
    struct EventData {
        std::string name;
        std::chrono::high_resolution_clock::time_point start_time;
        std::vector<double> times;  // 历史耗时记录
        uint64_t call_count = 0;
        double total_time_ms = 0.0;
        double min_time_ms = std::numeric_limits<double>::max();
        double max_time_ms = 0.0;
    };

    wgpu::Device m_device;
    bool m_enabled = true;
    bool m_supports_timestamp = false;

    // CPU 计时
    std::unordered_map<std::string, EventData> m_events;
    std::vector<std::string> m_event_stack;  // 事件栈，支持嵌套

    // 帧统计
    std::chrono::high_resolution_clock::time_point m_frame_start_time;
    std::chrono::high_resolution_clock::time_point m_last_frame_time;
    uint64_t m_frame_number = 0;
    std::vector<FrameStats> m_frame_history;
    static constexpr size_t MAX_FRAME_HISTORY = 300;  // 保留最近300帧

    // GPU 查询（如果支持）
    wgpu::QuerySet m_timestamp_query_set;
    wgpu::Buffer m_query_buffer;
    uint32_t m_query_index = 0;
    static constexpr uint32_t MAX_QUERIES = 256;

    void initializeTimestampQueries();
    double getCurrentTimeMs() const;
};

/**
 * @brief RAII 风格的性能测量辅助类
 *
 * 用法：
 * {
 *     ProfileScope scope(profiler, "MyOperation");
 *     // 执行操作
 * } // 自动结束测量
 */
class ProfileScope {
public:
    ProfileScope(GPUProfiler& profiler, const std::string& name)
        : m_profiler(profiler) {
        m_profiler.beginEvent(name);
    }

    ~ProfileScope() {
        m_profiler.endEvent();
    }

    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    GPUProfiler& m_profiler;
};

// 便捷宏定义
#define PROFILE_SCOPE(profiler, name) ProfileScope _profile_scope_##__LINE__(profiler, name)
#define PROFILE_FUNCTION(profiler) ProfileScope _profile_scope_##__LINE__(profiler, __FUNCTION__)

} // namespace ClipEngine
