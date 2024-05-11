#pragma once

#include "reactor/io/io.hh"
#include "reactor/io/file.hh"
#include "reactor/io/socket.hh"
#include "reactor/reactor.hh"
#include "reactor/task.hh"
#include "reactor/coroutine.hh"
#include "reactor/timer.hh"
#include "reactor/sync.hh"

#include "common/sink.hh"
#include "common/console.hh"

#include <cxxopts.hpp>

#include <concepts>

namespace corey {

using ParseResult = cxxopts::ParseResult;
using OptionAdder = cxxopts::OptionAdder;

struct ApplicationInfo {
    std::string name = "corey";
    std::string description = "A simple coroutine-based application";
    std::string version = "0.1.0";
};

template<typename Func, typename... Args>
concept MainFunc = requires(Func func, ParseResult result, Args&&... args) {
    { func(result, std::forward<Args>(args)...) } -> std::same_as<Future<int>>;
};

class Application {
public:
    Application(int argc, char* argv[], ApplicationInfo&& info = {});
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;
    ~Application();

    OptionAdder add_options();
    
    template<typename... Args>
    void add_positional_options(Args&&... args);

    void set_positional_help(const std::string& help);

    template<typename Func, typename... Args>
    requires MainFunc<Func, Args...>
    int run(Func&& func, Args&&... args);

private:

    ParseResult get_parse_result();

    int run(Future<int>&& task);

    Reactor _reactor;
    IoEngine _ioEngine;

    ApplicationInfo _info;
    int _argc;
    char** _argv;
    cxxopts::Options _options;
};

inline cxxopts::OptionAdder Application::add_options() {
    return _options.add_options();
}

template<typename... Args>
inline void Application::add_positional_options(Args&&... args) {
    _options.parse_positional(std::forward<Args>(args)...);
}

inline void Application::set_positional_help(const std::string& help) {
    _options.positional_help(help);
}

template<typename Func, typename... Args>
requires MainFunc<Func, Args...>
inline int Application::run(Func&& func, Args&&... args) {
    auto parse_results = get_parse_result();
    if (parse_results.count("help")) {
        auto sink = Sink::make<Console>();
        sink.write(fmt::format("{}\n", _options.help()));
        return 0;
    }
    if (parse_results.count("version")) {
        auto sink = Sink::make<Console>();
        sink.write(fmt::format("{}\n", _info.version));
        return 0;
    }
    return run(func(parse_results, std::forward<Args>(args)...));
}


} // namespace corey
