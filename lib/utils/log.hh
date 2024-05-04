#pragma once

#include "common/sink.hh"
#include "common/macro.hh"

#include <fmt/core.h>

#include <string>
#include <string_view>
#include <utility>

namespace corey {

class Log {
public:
    enum class Level {
        debug = 0,
        info = 1,
        warn = 2,
        error = 3
    };

    std::string get_name() const { return name; }
    Level get_level() const { return filter_level; }
    const Sink<>& get_sink() const { return sink; }

    Level set_level(Level);
    Sink<> set_sink(Sink<>&&);

    Log(std::string_view name);
    Log(std::string_view, Level);
    Log(std::string_view, Sink<>&&);
    Log(std::string_view, Level, Sink<>&&);
    Log(const Log&) = delete;
    Log &operator=(const Log&) = delete;
    Log(Log&&) = delete;
    Log &operator=(Log&&) = delete;
    ~Log();

    template<typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args) const {
        log(Level::debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) const {
        log(Level::info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args&&... args) const {
        log(Level::warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) const {
        log(Level::error, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log(Level level, fmt::format_string<Args...> fmt, Args&&... args) const {
        if (is_active(level)) {
            put(level, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

private:
    std::string name;
    Level filter_level;
    Sink<> sink;

    bool is_active(Level) const;
    void put(Log::Level, std::string_view) const;
};

void log_orphaned_exception(std::exception_ptr);

} // namespace corey

template<>
struct fmt::formatter<corey::Log::Level> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    
    template<typename FormatContext>
    auto format(corey::Log::Level const& level, FormatContext& ctx) {
        switch(level) {
            case corey::Log::Level::debug:
                return fmt::format_to(ctx.out(), "debug");
            case corey::Log::Level::info:
                return fmt::format_to(ctx.out(), "info");
            case corey::Log::Level::warn:
                return fmt::format_to(ctx.out(), "warn");
            case corey::Log::Level::error:
                return fmt::format_to(ctx.out(), "error");
            default:
                COREY_UNREACHABLE();
        }
    }
};
