#pragma once

#include <string>
#include <span>
#include <map>

#include <fmt/core.h>

namespace corey::http {

struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;

    void set_request(std::span<const char> req);
    void set_header(std::span<const char> header);

};

Request parse_request(std::span<const char> request);

} // namespace corey::http

template<typename Key, typename Value>
struct fmt::formatter<std::map<Key, Value>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const std::map<Key, Value>& map, FormatContext& ctx) {
        format_to(ctx.out(), "{{\n");
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (it != map.begin()) {
                format_to(ctx.out(), "\n");
            }
            format_to(ctx.out(), "    {}: {}", it->first, it->second);
        }
        return format_to(ctx.out(), "\n}}");
    }
};

template<>
struct fmt::formatter<corey::http::Request> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const corey::http::Request& request, FormatContext& ctx) {
        return format_to(ctx.out(),
            "Request:\n"
            "  Method: {}\n"
            "  Path: {}\n"
            "  Headers: {}\n"
            "  Body: {}",
            request.method, request.path, request.headers, request.body);
    }
};
