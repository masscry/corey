#pragma once

#include <string_view>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <fmt/format.h>

#define COREY_UNREACHABLE __builtin_unreachable

#define COREY_ASSERT(EXPR) do { \
    if (!(EXPR)) { \
        corey::panic("{}:{}: assertion failed ({})", __FILE__, __LINE__, #EXPR); \
    } \
} while (0)

#define COREY_ASSERT_MSG(EXPR, FMT, ...) do { \
    if (!(EXPR)) { \
        corey::panic("{}:{}: assertion failed ({}) - " FMT, __FILE__, __LINE__, #EXPR, __VA_ARGS__); \
    } \
} while (0)

namespace corey {

template<typename Ptr>
using IntrusivePtr = boost::intrusive_ptr<Ptr>;

template<typename Data>
concept Enum = std::is_enum_v<Data>;

// in std from C++23
template<Enum Data>
auto to_underlying(Data val) {
    return static_cast<std::underlying_type_t<Data>>(val);
}

namespace internal {

[[noreturn]]
void panic(std::string_view);

}

template<typename... Args>
[[noreturn]]
void panic(fmt::format_string<Args...> fmt, Args&&... args) {
    internal::panic(fmt::format(fmt, std::forward<Args>(args)...));
}

} // namespace corey
