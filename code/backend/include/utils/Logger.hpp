#pragma once

#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>

using namespace std;

namespace alphaforge {

enum class LogLevel { Debug, Info, Warn, Error };

[[nodiscard]] inline string_view as_string(LogLevel l) noexcept {
    switch (l) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

// Minimal, dependency free, thread safe logger. A single global sink is enough
// for a CLI application; the mutex guarantees lines are not interleaved when
// the analytics and risk layers log from worker threads.
class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) noexcept { min_level_ = level; }
    [[nodiscard]] LogLevel level() const noexcept { return min_level_; }

    void log(LogLevel level,
             string_view message,
             const source_location& loc = source_location::current()) {
        if (level < min_level_) {
            return;
        }
        const auto now = chrono::system_clock::now();
        scoped_lock lock(mutex_);
        clog << format("[{:%Y-%m-%d %H:%M:%S}] [{:<5}] {} ({}:{})\n",
                                 chrono::floor<chrono::seconds>(now),
                                 as_string(level),
                                 message,
                                 short_file(loc.file_name()),
                                 loc.line());
    }

    void debug(string_view m,
               const source_location& l = source_location::current()) {
        log(LogLevel::Debug, m, l);
    }
    void info(string_view m,
              const source_location& l = source_location::current()) {
        log(LogLevel::Info, m, l);
    }
    void warn(string_view m,
              const source_location& l = source_location::current()) {
        log(LogLevel::Warn, m, l);
    }
    void error(string_view m,
               const source_location& l = source_location::current()) {
        log(LogLevel::Error, m, l);
    }

private:
    Logger() = default;

    static string_view short_file(string_view path) noexcept {
        const auto pos = path.find_last_of("/\\");
        return pos == string_view::npos ? path : path.substr(pos + 1);
    }

    LogLevel   min_level_{LogLevel::Info};
    mutex mutex_;
};

} // namespace alphaforge
