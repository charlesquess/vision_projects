#pragma once

#include <string>
#include <memory>

/* 日志级别枚举 */
enum class LogLevel {
    Debug,  /* 调试信息 */
    Info,   /* 常规信息 */
    Warn,   /* 警告 */
    Error,  /* 错误 */
    Fatal   /* 致命错误 */
};

/* 日志记录器 - 单例模式，同时输出到控制台和文件 */
class Logger {
public:
    static Logger& instance();  /* 获取全局单例实例 */

    void set_level(LogLevel level);          /* 设置日志级别，低于此级别的日志被过滤 */
    void set_log_file(const std::string& filepath);  /* 设置日志文件路径 */

    /* 各级别日志输出方法 */
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

    void log(LogLevel level, const std::string& message);  /* 通用日志方法 */

private:
    Logger();
    ~Logger();
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/* 便捷宏，无需每次写 Logger::instance() */
#define LOG_DEBUG(msg)    Logger::instance().debug(msg)
#define LOG_INFO(msg)     Logger::instance().info(msg)
#define LOG_WARN(msg)     Logger::instance().warn(msg)
#define LOG_ERROR(msg)    Logger::instance().error(msg)
#define LOG_FATAL(msg)    Logger::instance().fatal(msg)
