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

Log::Level Log::set_level(Log::Level level) {
    return std::exchange(filter_level, level);
}

Sink<> Log::set_sink(Sink<>&& new_sink) {
    return std::exchange(sink, std::move(new_sink));
}

Log::Log(std::string_view name) : Log(name, Level::info) { ; }
Log::Log(std::string_view name, Level level) : Log(name, level, Sink<Console>::make()) { ; }
Log::Log(std::string_view name, Sink<>&& new_sink) : Log(name, Level::info, std::move(new_sink)) { ; }

Log::Log(std::string_view name, Level level, Sink<>&& new_sink) : name(name) {
    COREY_ASSERT(!name.empty());
    set_level(level);
    set_sink(std::move(new_sink));
}

Log::~Log() { ; }

bool Log::is_active(Log::Level level) const {
    return level >= filter_level;
}

void Log::put(Log::Level level, std::string_view text) const {
    sink.write(fmt::format("[{}][{}] {}: {}\n", std::chrono::system_clock::now(), level, this->name, text));
}

} // namespace corey