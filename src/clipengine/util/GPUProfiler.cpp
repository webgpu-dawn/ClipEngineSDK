#include "GPUProfiler.h"
#include <iostream>
#include <iomanip>

namespace ClipEngine {

GPUProfiler::GPUProfiler(const wgpu::Device& device)
    : m_device(device) {
    // 检查是否支持 timestamp query
    // 注意：WebGPU 的 timestamp-query 特性需要显式启用
    // 这里我们先使用 CPU 计时，GPU 计时可以后续添加
    m_supports_timestamp = false;

    m_frame_history.reserve(MAX_FRAME_HISTORY);
}

GPUProfiler::~GPUProfiler() {
}

void GPUProfiler::beginEvent(const std::string& name) {
    if (!m_enabled) return;

    auto& event = m_events[name];
    event.name = name;
    event.start_time = std::chrono::high_resolution_clock::now();

    m_event_stack.push_back(name);
}

void GPUProfiler::endEvent() {
    if (!m_enabled || m_event_stack.empty()) return;

    auto end_time = std::chrono::high_resolution_clock::now();
    std::string name = m_event_stack.back();
    m_event_stack.pop_back();

    auto& event = m_events[name];
    double elapsed_ms = std::chrono::duration<double, std::milli>(
        end_time - event.start_time).count();

    // 更新统计信息
    event.call_count++;
    event.total_time_ms += elapsed_ms;
    if (elapsed_ms < event.min_time_ms) event.min_time_ms = elapsed_ms;
    if (elapsed_ms > event.max_time_ms) event.max_time_ms = elapsed_ms;
    event.times.push_back(elapsed_ms);

    // 只保留最近 100 次记录
    if (event.times.size() > 100) {
        event.times.erase(event.times.begin());
    }
}

void GPUProfiler::beginFrame() {
    if (!m_enabled) return;

    m_frame_start_time = std::chrono::high_resolution_clock::now();
    beginEvent("Frame");
}

void GPUProfiler::endFrame() {
    if (!m_enabled) return;

    endEvent(); // End "Frame" event

    auto frame_end_time = std::chrono::high_resolution_clock::now();
    double frame_time_ms = std::chrono::duration<double, std::milli>(
        frame_end_time - m_frame_start_time).count();

    // 收集帧统计
    FrameStats stats;
    stats.frame_number = m_frame_number++;
    stats.frame_time_ms = frame_time_ms;

    // 提取渲染和拷贝时间（如果有记录）
    if (m_events.find("Render") != m_events.end() && !m_events["Render"].times.empty()) {
        stats.render_time_ms = m_events["Render"].times.back();
    } else {
        stats.render_time_ms = 0.0;
    }

    if (m_events.find("Copy") != m_events.end() && !m_events["Copy"].times.empty()) {
        stats.copy_time_ms = m_events["Copy"].times.back();
    } else {
        stats.copy_time_ms = 0.0;
    }

    m_frame_history.push_back(stats);

    // 限制历史记录大小
    if (m_frame_history.size() > MAX_FRAME_HISTORY) {
        m_frame_history.erase(m_frame_history.begin());
    }

    m_last_frame_time = frame_end_time;
}

void GPUProfiler::beginGPUQuery(wgpu::CommandEncoder encoder, const std::string& name) {
    // TODO: 实现 GPU timestamp query
    // 需要在创建设备时启用 timestamp-query 特性
    if (!m_supports_timestamp) return;
}

void GPUProfiler::endGPUQuery(wgpu::CommandEncoder encoder) {
    if (!m_supports_timestamp) return;
}

std::vector<GPUProfiler::ProfileResult> GPUProfiler::getResults() const {
    std::vector<ProfileResult> results;

    for (const auto& pair : m_events) {
        const auto& event = pair.second;
        if (event.call_count == 0) continue;

        ProfileResult result;
        result.name = pair.first;
        result.cpu_time_ms = event.total_time_ms;
        result.gpu_time_ms = 0.0;  // TODO: 添加 GPU 计时
        result.call_count = event.call_count;
        result.avg_time_ms = event.total_time_ms / event.call_count;
        result.min_time_ms = event.min_time_ms;
        result.max_time_ms = event.max_time_ms;

        results.push_back(result);
    }

    // Sort by average time - need to add <algorithm> include
    // std::sort(results.begin(), results.end(),
    //     [](const ProfileResult& a, const ProfileResult& b) {
    //         return a.avg_time_ms > b.avg_time_ms;
    //     });

    return results;
}

std::vector<GPUProfiler::FrameStats> GPUProfiler::getFrameStats(size_t frame_count) const {
    if (m_frame_history.empty()) {
        return {};
    }

    size_t count = (frame_count < m_frame_history.size()) ? frame_count : m_frame_history.size();
    return std::vector<FrameStats>(
        m_frame_history.end() - count,
        m_frame_history.end()
    );
}

void GPUProfiler::printReport() const {
    std::cout << "\n"
              << "========================================\n"
              << "       GPU Performance Report\n"
              << "========================================\n\n";

    auto results = getResults();

    if (results.empty()) {
        std::cout << "No profiling data available.\n";
        return;
    }

    // 打印性能指标表格
    std::cout << std::left
              << std::setw(30) << "Event Name"
              << std::right
              << std::setw(12) << "Calls"
              << std::setw(12) << "Total(ms)"
              << std::setw(12) << "Avg(ms)"
              << std::setw(12) << "Min(ms)"
              << std::setw(12) << "Max(ms)"
              << "\n";

    std::cout << std::string(90, '-') << "\n";

    for (const auto& result : results) {
        std::cout << std::left << std::setw(30) << result.name
                  << std::right << std::fixed << std::setprecision(2)
                  << std::setw(12) << result.call_count
                  << std::setw(12) << result.cpu_time_ms
                  << std::setw(12) << result.avg_time_ms
                  << std::setw(12) << result.min_time_ms
                  << std::setw(12) << result.max_time_ms
                  << "\n";
    }

    std::cout << std::string(90, '-') << "\n";

    // 帧统计
    if (!m_frame_history.empty()) {
        auto recent_frames = getFrameStats(60);
        if (!recent_frames.empty()) {
            double avg_frame_time = 0.0;
            double avg_render_time = 0.0;
            double avg_copy_time = 0.0;

            for (const auto& frame : recent_frames) {
                avg_frame_time += frame.frame_time_ms;
                avg_render_time += frame.render_time_ms;
                avg_copy_time += frame.copy_time_ms;
            }

            avg_frame_time /= recent_frames.size();
            avg_render_time /= recent_frames.size();
            avg_copy_time /= recent_frames.size();

            std::cout << "\nFrame Statistics (last " << recent_frames.size() << " frames):\n";
            std::cout << "  Average FPS: " << std::fixed << std::setprecision(1)
                      << (1000.0 / avg_frame_time) << "\n";
            std::cout << "  Average Frame Time: " << std::setprecision(2)
                      << avg_frame_time << " ms\n";
            std::cout << "  Average Render Time: " << avg_render_time << " ms\n";
            std::cout << "  Average Copy Time: " << avg_copy_time << " ms\n";
        }
    }

    std::cout << "\n========================================\n\n";
}

void GPUProfiler::reset() {
    m_events.clear();
    m_event_stack.clear();
    m_frame_history.clear();
    m_frame_number = 0;
}

double GPUProfiler::getCurrentTimeMs() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(
        now.time_since_epoch()).count();
}

void GPUProfiler::initializeTimestampQueries() {
    // TODO: 初始化 timestamp query set
    // 需要在支持的设备上创建 query set 和 buffer
}

} // namespace ClipEngine
