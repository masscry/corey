#pragma once

#include "reactor/reactor.hh"
#include "reactor/task.hh"

#include <liburing.h>

#include <linux/time_types.h>

namespace corey {

constexpr auto max_events = 128u;
constexpr int invalid_fd = -1;

class Context {
    friend class IoEngine;
public:
    Context() = default;
    Context(Context&& other) noexcept : _tag(other._tag) { other._tag = 0; }
    Context& operator=(Context&& rhs) noexcept {
        if (this != &rhs) {
            this->~Context();
            new(this) Context(std::move(rhs));
        }
        return *this;
    }
    ~Context() = default;

    Future<> cancel();

private:
    void* _tag = 0;
};

class IoEngine {
public:

    static
    IoEngine& instance();

    IoEngine(Reactor&);
    IoEngine(const IoEngine& other) = delete;
    IoEngine& operator=(const IoEngine& other) = delete;
    IoEngine(IoEngine&& other) noexcept = delete;
    IoEngine& operator=(IoEngine&& other) noexcept = delete;
    ~IoEngine();

    Future<int> open(const char* path, int flags, Context* = nullptr);
    Future<int> open(const char* path, int flags, mode_t mode, Context* = nullptr);
    Future<int> fsync(int fd, Context* = nullptr);
    Future<int> fdatasync(int fd, Context* = nullptr);
    Future<int> read(int fd, uint64_t offset, std::span<char>, Context* = nullptr);
    Future<int> readv(int fd, uint64_t offset, std::span<iovec>, Context* = nullptr);
    Future<int> write(int fd, uint64_t offset, std::span<const char>, Context* = nullptr);
    Future<int> writev(int fd, uint64_t offset, std::span<const iovec>, Context* = nullptr);
    Future<int> send(int fd, std::span<const char>, int flags, Context* = nullptr);
    Future<int> recv(int fd, std::span<char>, int flags, Context* = nullptr);
    Future<int> close(int fd, Context* = nullptr);
    Future<int> timeout(__kernel_timespec*, Context* = nullptr);
    Future<int> socket(int domain, int type, int protocol, Context* = nullptr);
    Future<int> connect(int fd, const sockaddr* addr, socklen_t addrlen, Context* = nullptr);
    Future<int> accept(int fd, sockaddr* addr, socklen_t* addrlen, Context* = nullptr);
    Future<int> setsockopt(int fd, int level, int optname, const void* optval, socklen_t optlen, Context* = nullptr);
    Future<int> bind(int fd, const sockaddr* addr, socklen_t addrlen, Context* = nullptr);
    Future<int> listen(int fd, int backlog, Context* = nullptr);
    Future<int> cancel(const Context&);

private:

    void submit_pending();
    void complete_ready();

    template<typename Func, typename... Args>
    inline Promise<int>* prepare(Func&&, Context*, Args&&...);

    template<typename Func, typename... Args>
    inline Future<int> posix_call(Func&&, Args&&...);

    io_uring _ring;
    Defer<> _poll_routine;
    int _pending = 0;
    int _inflight = 0;
    Reactor& _reactor;
};

} // namespace corey
