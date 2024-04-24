#pragma once

#include "utils/log.hh"

#include <fmt/core.h>

namespace corey {

template<typename Data>
concept Enum = std::is_enum_v<Data>;

// in std from C++23
template<Enum Data>
auto to_underlying(Data val) {
    return static_cast<std::underlying_type_t<Data>>(val);
}

template<typename... Args>
[[noreturn]]
void panic(fmt::format_string<Args...> fmt, Args&&... args) {
    static Log log("panic");
    log.error(fmt, std::forward<Args>(args)...);
    __builtin_trap();
}

} // namespace corey
