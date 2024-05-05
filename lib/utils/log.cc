#include "sink.hh"
#include "console.hh"
#include "utils/common.hh"
#include "utils/log.hh"

#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/chrono.h"

#include <atomic>
#include <chrono>
#include <utility>

namespace corey {

namespace {

Sink<> default_sink = Sink<Console>::make();
Log::Level default_level = Log::Level::info;

} // namespace

Sink<> Log::set_default_sink(const Sink<>& new_sink) {
    return std::exchange(default_sink, std::move(new_sink));
}

Sink<> Log::get_default_sink() {
    return default_sink;
}

Log::Level Log::set_default_level(Log::Level level) {
    return std::exchange(default_level, level);
}

Log::Level Log::get_default_level() {
    return default_level;
}

Log::Level Log::set_level(Log::Level level) {
    return std::exchange(filter_level, level);
}

Sink<> Log::set_sink(const Sink<>& new_sink) {
    return std::exchange(sink, new_sink);
}

Log::Log(std::string_view name) : Log(name, get_default_level(), get_default_sink()) {}
Log::Log(std::string_view name, Level level) : Log(name, level, get_default_sink()) {}
Log::Log(std::string_view name, const Sink<>& new_sink) : Log(name, get_default_level(), new_sink) {}
Log::Log(std::string_view name, Level level, const Sink<>& new_sink)
    : name(name)
    , filter_level(level)
    , sink(new_sink)
{
    COREY_ASSERT(!name.empty());
}

bool Log::is_active(Log::Level level) const {
    return level >= filter_level;
}

void Log::put(Log::Level level, std::string_view text) const {
    sink.write(fmt::format("[{}][{}] {}: {}\n", std::chrono::system_clock::now(), level, this->name, text));
}

void log_orphaned_exception(std::exception_ptr exp) {
    static Log log("orphan");
    try {
        std::rethrow_exception(exp);
    } catch (const std::exception& e) {
        log.error("unhandled exception: {}", e.what());
    } catch (...) {
        log.error("unhandled exception: unknown");
    }
}

} // namespace corey