#include "io.hh"

#include <cstdint>
#include <liburing.h>
#include "fmt/ostream.h"
#include "liburing/io_uring.h"

#include "common/macro.hh"
#include "reactor/future.hh"
#include "reactor/reactor.hh"
#include "reactor/task.hh"
#include "reactor/coroutine.hh"
#include "utils/log.hh"
#include "utils/common.hh"

#include <exception>
#include <fcntl.h>
#include <system_error>
#include <span>
#include <utility>
#include <memory>

#include <fmt/std.h>

#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <unistd.h>

namespace corey {

namespace {

Log logger("io");

static_assert(
    sizeof(Promise<int>) == sizeof(io_uring_sqe::user_data),
    "Promise<int> must be the same size as io_uring data"
);

IoEngine* _instance = nullptr;

} // namespace

IoEngine& IoEngine::instance() {
    if (!_instance) {
        panic("IoEngine not initialized");
    }
    return *_instance;
}

IoEngine::IoEngine(Reactor& reactor) : _reactor(reactor) {
    if (_instance) {
        panic("IoEngine already initialized");
    }

    if (int ret = io_uring_queue_init(max_events, &_ring, 0); ret != 0) {
        throw std::system_error(-ret, std::system_category(), "io_uring_queue_init failed");
    }
    _poll_routine = reactor.add_routine(make_routine([this] {
        submit_pending();
        complete_ready();
    }));
    _epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (_epoll_fd < 0) {
        throw std::system_error(errno, std::system_category(), "epoll_create1 failed");
    }
    _instance = this;
}

IoEngine::~IoEngine() {
    COREY_ASSERT(_pending == 0);
    COREY_ASSERT(_inflight == 0);
    ::close(_epoll_fd);
    io_uring_queue_exit(&_ring);
    _instance = nullptr;
}

Future<int> IoEngine::open(const char* path, int flags) {
    if ((flags & O_CREAT) || (flags & O_TMPFILE)) {
        return make_exception_future<int>(std::make_exception_ptr(std::invalid_argument("missing mode for open")));
    }
    return open(path, flags, 0);
}

Future<int> IoEngine::open(const char* path, int flags, mode_t mode) {
    return prepare(io_uring_prep_openat, AT_FDCWD, path, flags, mode)->get_future();
}

Future<int> IoEngine::fsync(int fd) {
    return prepare(io_uring_prep_fsync, fd, 0)->get_future();
}

Future<int> IoEngine::fdatasync(int fd) {
    return prepare(io_uring_prep_fsync, fd, IORING_FSYNC_DATASYNC)->get_future();
}

Future<int> IoEngine::read(int fd, uint64_t offset, std::span<char> data) {
    return prepare(io_uring_prep_read, fd, data.data(), data.size(), offset)->get_future();
}

Future<int> IoEngine::readv(int fd, uint64_t offset, std::span<iovec> iov) {
    return prepare(io_uring_prep_readv, fd, iov.data(), iov.size(), offset)->get_future();
}

Future<int> IoEngine::writev(int fd, uint64_t offset, std::span<const iovec> iov) {
    return prepare(io_uring_prep_writev, fd, iov.data(), iov.size(), offset)->get_future();
}

Future<int> IoEngine::write(int fd, uint64_t offset, std::span<const char> data) {
    return prepare(io_uring_prep_write, fd, data.data(), data.size(), offset)->get_future();
}

Future<int> IoEngine::send(int fd, std::span<const char> buf, int flags) {
    return prepare(io_uring_prep_send, fd, buf.data(), buf.size_bytes(), flags)->get_future();
}

Future<int> IoEngine::recv(int fd, std::span<char> buf, int flags) {
    return prepare(io_uring_prep_recv, fd, buf.data(), buf.size_bytes(), flags)->get_future();
}

Future<int> IoEngine::close(int fd) {
    return prepare(io_uring_prep_close, fd)->get_future();
}

Future<int> IoEngine::timeout(__kernel_timespec* ts) {
    return prepare(io_uring_prep_timeout, ts, 0, IORING_TIMEOUT_ABS)->get_future();
}

Future<int> IoEngine::socket(int domain, int type, int protocol) {
    return prepare(io_uring_prep_socket, domain, type, protocol, 0)->get_future();
}

Future<int> IoEngine::connect(int fd, const sockaddr* addr, socklen_t addrlen) {
    return prepare(io_uring_prep_connect, fd, addr, addrlen)->get_future();
}

Future<int> IoEngine::accept(int fd, sockaddr* addr, socklen_t* addrlen) {
    return prepare(io_uring_prep_accept, fd, addr, addrlen, 0)->get_future();
}

Future<int> IoEngine::poll_add(int fd, uint32_t events) {
    return prepare(io_uring_prep_poll_add, fd, events)->get_future();        
}

Future<int> IoEngine::setsockopt(int fd, int level, int optname, const void* optval, socklen_t optlen) {
    return posix_call(::setsockopt, fd, level, optname, optval, optlen);
}

Future<int> IoEngine::bind(int fd, const sockaddr* addr, socklen_t addrlen) {
    return posix_call(::bind, fd, addr, addrlen);
}

Future<int> IoEngine::listen(int fd, int backlog) {
    return posix_call(::listen, fd, backlog);
}

Future<int> IoEngine::signalfd(int fd, const sigset_t* mask, int flags) {
    _handle_poller = run_poller();
    return posix_call(::signalfd, fd, mask, flags);
}

Future<int> IoEngine::epoll_ctl(int op, int fd, uint32_t event, void* data) {
    epoll_event evt {
        .events = event,
        .data = {
            .ptr = data
        }
    };
    return posix_call(::epoll_ctl, _epoll_fd, op, fd, &evt);
}

Future<int> IoEngine::epoll_wait(int fd, std::span<epoll_event> events, int timeout) {
    return posix_call(::epoll_wait, fd, events.data(), events.size(), timeout);
}

void IoEngine::submit_pending() {
    while(_pending > 0) {
        int ret = io_uring_submit(&_ring);
        if (ret < 0) {
            logger.error("io_uring_submit failed: {}", std::system_error(-ret, std::system_category()));
            return;
        }
        _pending -= ret;
        _inflight += ret;
    }
}

void IoEngine::complete_ready() {
    constexpr auto complete_cqe = [](IoEngine& engine, io_uring_cqe* cqe) {
        auto comp = reinterpret_cast<Promise<int>*>(&cqe->user_data);
        comp->set(cqe->res);
        comp->~Promise();
        io_uring_cqe_seen(&engine._ring, cqe);
        --engine._inflight;
    };

    io_uring_cqe *cqe;
    if (!_reactor.has_progress() && (_inflight > 0)) {
        auto err = io_uring_wait_cqe(&_ring, &cqe);
        COREY_ASSERT_MSG(err == 0, "io_uring_wait_cqe failed: {}", std::system_error(-err, std::system_category()));
        complete_cqe(*this, cqe);
    }

    while (true) {
        auto err = io_uring_peek_cqe(&_ring, &cqe);
        if (err == -EAGAIN) {
            break;
        }
        COREY_ASSERT_MSG(err == 0, "io_uring_peek_cqe failed: {}", std::system_error(-err, std::system_category()));
        complete_cqe(*this, cqe);
    }
}

Future<> IoEngine::run_poller() {
    while (true) {
        auto ret = co_await poll_add(_epoll_fd, POLLIN);
        if (ret < 0) {
            co_await std::make_exception_ptr(std::system_error(-ret, std::system_category(), "poll_add failed"));
        }
        epoll_event events[max_events];
        ret = co_await epoll_wait(_epoll_fd, std::span(events, max_events), -1);
        if (ret < 0) {
            co_await std::make_exception_ptr(std::system_error(-ret, std::system_category(), "epoll_wait failed"));
        }
        fmt::print("epoll_wait returned {}\n", ret);
        for (auto evt: std::span(events, ret)) {
            auto promise = reinterpret_cast<Promise<uint32_t>*>(evt.data.ptr);
            promise->set(static_cast<uint32_t>(evt.events));
        }
    }
}

template<typename Func, typename... Args>
inline Promise<int>* IoEngine::prepare(Func&& func, Args&&... args) {
    if (auto sqe = io_uring_get_sqe(&_ring)) {
        std::invoke(std::forward<Func>(func), sqe, std::forward<Args>(args)...);
        auto comp = new (reinterpret_cast<void*>(&sqe->user_data)) Promise<int>;
        ++_pending;
        return comp;
    }
    panic("no sqe available in io_uring");
}

template<typename Func, typename... Args>
inline Future<int> IoEngine::posix_call(Func&& func, Args&&... args) {
    auto ret = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    if (ret < 0) {
        return make_ready_future<int>(-errno);
    }
    return make_ready_future<int>(ret);
}

} // namespace corey