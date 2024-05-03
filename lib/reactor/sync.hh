#pragma once

#include "common/defer.hh"
#include "reactor/future.hh"

#include <queue>

namespace corey {

class Semaphore {
public:

    Semaphore(int count);
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Future<Defer<>> wait();

    int count() const { return _count; }

private:

    Defer<> signal_later();

    int _count;
    std::queue<Promise<Defer<>>> _waiters;
};


} // namespace corey
