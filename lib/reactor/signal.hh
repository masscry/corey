#pragma once

#include "reactor/task.hh"
#include "reactor/future.hh"
#include "common/defer.hh"

namespace corey {

using SignalHandler = Executable<Future<>, int>;

Future<> handle_signals(int signum, SignalHandler handler);

template <typename Func>
auto make_signal_handler(Func&& func) {
    struct SimpleSignalHandler final : public AbstractExecutable<Future<>, int> {
        Func _func;
        explicit SimpleSignalHandler(Func&& func) : _func(std::forward<Func>(func)) {}
        Future<> execute(int&& signum) override {
            return _func(signum);
        }
    };
    return SignalHandler::make<SimpleSignalHandler>(std::forward<Func>(func));
}

} // namespace corey
