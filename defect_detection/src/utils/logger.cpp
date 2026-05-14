#include "utils/logger.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>

/* 日志记录器内部实现 (PIMPL + 单例) */
class Logger::Impl {
public:
    Impl() : level_(LogLevel::Info) {}

    void set_level(LogLevel level) { level_ = level; }

    void set_log_file(const std::string& filepath) {
        file_stream_.open(filepath, std::ios::app);  /* 追加模式 */
    }

    /* 输出一条日志：格式 [时间] [级别] 消息 */
    void log(LogLevel level, const std::string& message) {
        if (level < level_) return;  /* 低于当前级别则过滤 */

        std::string level_str;
        switch (level) {
            case LogLevel::Debug: level_str = "DEBUG"; break;
            case LogLevel::Info:  level_str = "INFO";  break;
            case LogLevel::Warn:  level_str = "WARN";  break;
            case LogLevel::Error: level_str = "ERROR"; break;
            case LogLevel::Fatal: level_str = "FATAL"; break;
        }

        /* 生成带时间戳的格式化日志行 */
        auto now = std::time(nullptr);
        std::tm tm;
        localtime_s(&tm, &now);
        std::ostringstream oss;
        oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << "] [" << level_str << "] " << message;

        std::string formatted = oss.str();
        std::cout << formatted << std::endl;

        /* 同时写入日志文件 */
        if (file_stream_.is_open()) {
            file_stream_ << formatted << std::endl;
        }
    }

private:
    LogLevel level_;              /* 当前日志级别 */
    std::ofstream file_stream_;   /* 日志文件流 */
};

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::set_level(LogLevel level) { impl_->set_level(level); }
void Logger::set_log_file(const std::string& fp) { impl_->set_log_file(fp); }
void Logger::debug(const std::string& m) { impl_->log(LogLevel::Debug, m); }
void Logger::info(const std::string& m)  { impl_->log(LogLevel::Info, m); }
void Logger::warn(const std::string& m)  { impl_->log(LogLevel::Warn, m); }
void Logger::error(const std::string& m) { impl_->log(LogLevel::Error, m); }
void Logger::fatal(const std::string& m) { impl_->log(LogLevel::Fatal, m); }
void Logger::log(LogLevel l, const std::string& m) { impl_->log(l, m); }

Logger::Logger() : impl_(std::make_unique<Impl>()) {}
Logger::~Logger() = default;
