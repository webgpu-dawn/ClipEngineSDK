#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace ClipEngine {

// ClipEngine 日志封装类（基于 spdlog）
class CeLogger {
public:
    // 获取单例实例
    static CeLogger& instance() {
        static CeLogger logger;
        return logger;
    }

    // 获取底层的 spdlog logger
    std::shared_ptr<spdlog::logger> get() {
        return logger_;
    }

    // 设置日志级别
    void setLevel(spdlog::level::level_enum level) {
        logger_->set_level(level);
    }

    // 设置日志输出格式
    // 默认格式: [时间] [级别] 消息
    void setPattern(const std::string& pattern) {
        logger_->set_pattern(pattern);
    }

private:
    CeLogger() {
        // 创建带颜色的控制台 logger
        logger_ = spdlog::stdout_color_mt("ClipEngine");
        
        // 设置默认日志级别为 info
        logger_->set_level(spdlog::level::info);
        
        // 设置默认格式：[时间] [级别] 消息
        logger_->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        
        // 立即刷新（确保日志实时输出）
        logger_->flush_on(spdlog::level::trace);
    }

    ~CeLogger() {
        spdlog::shutdown();
    }

    CeLogger(const CeLogger&) = delete;
    CeLogger& operator=(const CeLogger&) = delete;

    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace ClipEngine

// 便捷的日志宏（支持格式化字符串）
#define LOG_TRACE(...)    SPDLOG_LOGGER_TRACE(::ClipEngine::CeLogger::instance().get(), __VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG(::ClipEngine::CeLogger::instance().get(), __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_INFO(::ClipEngine::CeLogger::instance().get(), __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_WARN(::ClipEngine::CeLogger::instance().get(), __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(::ClipEngine::CeLogger::instance().get(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::ClipEngine::CeLogger::instance().get(), __VA_ARGS__)
