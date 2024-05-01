#pragma once

#include "reactor.hh"
#include <task.hh>

#include <liburing.h>

namespace corey {

constexpr auto max_events = 128u;

class IoEngine {
public:

    IoEngine(Reactor&);
    IoEngine(const IoEngine& other) = delete;
    IoEngine& operator=(const IoEngine& other) = delete;
    IoEngine(IoEngine&& other) noexcept = delete;
    IoEngine& operator=(IoEngine&& other) noexcept = delete;
    ~IoEngine();

    Future<int> read(int fd, uint64_t offset, std::span<char>);
    Future<int> write(int fd, uint64_t offset, std::span<const char>);
    Future<int> close(int fd);

private:

    void submit_pending();
    void complete_ready();

    template<typename Func, typename... Args>
    inline
    Promise<int>* prepare(Func func, Args&&... args);

    io_uring _ring;
    Defer<> _poll_routine;
    int _pending = 0;
};

} // namespace corey
