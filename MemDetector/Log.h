#pragma once
#include <chrono>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <mutex>
// 日志等级枚举
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

inline std::mutex& get_log_mutex() {
    static std::mutex mtx;
    return mtx;
}

// 获取当前时间戳字符串（跨平台实现）
inline std::string current_timestamp() noexcept {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    
    #ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t);
    #else
    localtime_r(&in_time_t, &tm_buf);
    #endif
    
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

#ifdef __cpp_lib_print
    #include <print>
    inline void Print(LogLevel level, const std::source_location& loc, auto&&... args) {
        std::lock_guard<std::mutex> lock(get_log_mutex());  
        std::print(stderr, "[{}] {} {}:{}: ", current_timestamp(), log_level_to_string(level), loc.file_name(), loc.line());
        std::print(stderr, std::forward<decltype(args)>(args)...);
        std::print(stderr, "\n");
    }
#else
    #include <iostream>
    #include <sstream>
    template<typename... Args>
    inline void Print(LogLevel level, const std::source_location& loc, Args&&... args) {
        std::lock_guard<std::mutex> lock(get_log_mutex());  
        std::cerr << "[" << current_timestamp() << "] "
                  << log_level_to_string(level) << " "
                  << loc.file_name() << ":" << loc.line() << ": ";
        (std::cerr << ... << std::forward<Args>(args)) << "\n";
    }
#endif


// 核心日志宏实现
#define LOG(level, ...) \
    do { \
        const auto loc = std::source_location::current(); \
        Print(level, loc, __VA_ARGS__); \
    } while(0) \

// 日志等级快捷宏
#define LOG_DEBUG(...)   LOG(LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)    LOG(LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...)    LOG(LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...)   LOG(LogLevel::Error, __VA_ARGS__)
#define LOG_CRITICAL(...)LOG(LogLevel::Critical, __VA_ARGS__)

// 辅助函数：日志等级转字符串
constexpr std::string_view log_level_to_string(LogLevel level) noexcept {
    switch(level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}
#include <fstream>
#include <filesystem>
#include <memory>

class Log {
public:
    // 单例模式（线程安全）
    static Log& Instance() {
        static Log instance;
        return instance;
    }

    // 设置日志文件路径（非必须，默认不写入文件）
    void SetLogFile(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (file_.is_open()) {
            file_.close();
        }
        file_.open(filepath, std::ios::app);
        if (!file_) {
            std::cerr << "Failed to open log file: " << filepath << std::endl;
        }
    }

    // 写入日志（线程安全）
    void Write(LogLevel level, const std::string& message, const std::source_location& loc) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (file_.is_open()) {
            file_ << "[" << current_timestamp() << "] "
                  << log_level_to_string(level) << " "
                  << loc.file_name() << ":" << loc.line() << ": "
                  << message << "\n";
            file_.flush();  // 立即刷新缓冲区
        }
    }

private:
    std::ofstream file_;
    std::mutex file_mutex_;

    Log() = default;
    ~Log() {
        if (file_.is_open()) {
            file_.close();
        }
    }
private:
    // 禁止拷贝和移动
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(Log&&) = delete;
};