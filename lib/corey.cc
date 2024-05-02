#include "corey.hh"
#include "console.hh"
#include "reactor/task.hh"
#include "sink.hh"

#include <cxxopts.hpp>

namespace corey {

Application::Application(int argc, char* argv[], ApplicationInfo&& info)
    : _ioEngine(_reactor)
    , _info(std::move(info))
    , _argc(argc)
    , _argv(argv)
    , _options(_info.name, _info.description) {

    _options.add_options()
        ("h,help", "Print help")
        ("v,version", "Print version");
}

Application::~Application() { ; }

cxxopts::ParseResult Application::get_parse_result() {
    if (_argc > 0) {
        return _options.parse(_argc, _argv);
    }
    char* argv[] = { _info.name.data() };
    return _options.parse(1, argv);
}

int Application::run(Future<int>&& task) {
    while (task.is_ready() == false) {
        _reactor.run();
    }
    return task.get();
}

} // namespace corey
