#include "sync.hh"
#include "reactor/future.hh"

namespace corey {

Semaphore::Semaphore(int count) : _count(count) {}

Future<Defer<>> Semaphore::wait() {
    if (_count > 0) {
        --_count;
        return make_ready_future<Defer<>>(signal_later());
    }
    auto promise = Promise<Defer<>>();
    auto future = promise.get_future();
    _waiters.push(std::move(promise));
    return future;
}

Defer<> Semaphore::signal_later() {
    return defer([this]() noexcept {
        if (_waiters.empty()) {
            _count++;
            return;
        }
        auto promise = std::move(_waiters.front());
        _waiters.pop();
        promise.set(signal_later());
    });
}

} // namespace corey
