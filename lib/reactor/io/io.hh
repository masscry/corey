#pragma once

#include "reactor/reactor.hh"
#include "reactor/task.hh"

#include <liburing.h>

#include <linux/time_types.h>

namespace corey {

constexpr auto max_events = 128u;
constexpr int invalid_fd = -1;

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

    Future<int> open(const char* path, int flags);
    Future<int> open(const char* path, int flags, mode_t mode);
    Future<int> fsync(int fd);
    Future<int> fdatasync(int fd);
    Future<int> read(int fd, uint64_t offset, std::span<char>);
    Future<int> readv(int fd, uint64_t offset, std::span<iovec>);
    Future<int> write(int fd, uint64_t offset, std::span<const char>);
    Future<int> writev(int fd, uint64_t offset, std::span<const iovec>);
    Future<int> send(int fd, std::span<const char>, int flags);
    Future<int> recv(int fd, std::span<char>, int flags);
    Future<int> close(int fd);
    Future<int> timeout(__kernel_timespec*);
    Future<int> socket(int domain, int type, int protocol);
    Future<int> connect(int fd, const sockaddr* addr, socklen_t addrlen);
    Future<int> accept(int fd, sockaddr* addr, socklen_t* addrlen);
    Future<int> setsockopt(int fd, int level, int optname, const void* optval, socklen_t optlen);
    Future<int> bind(int fd, const sockaddr* addr, socklen_t addrlen);
    Future<int> listen(int fd, int backlog);

private:

    void submit_pending();
    void complete_ready();

    template<typename Func, typename... Args>
    inline Promise<int>* prepare(Func&& func, Args&&... args);

    template<typename Func, typename... Args>
    inline Future<int> posix_call(Func&& func, Args&&... args);

    io_uring _ring;
    Defer<> _poll_routine;
    int _pending = 0;
    int _inflight = 0;
    Reactor& _reactor;
};

} // namespace corey
