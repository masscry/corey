#include "io.hh"

#include <cstdint>
#include <liburing.h>
#include "liburing/io_uring.h"

#include "common.hh"
#include "reactor/future.hh"
#include "reactor/reactor.hh"
#include "reactor/task.hh"
#include "reactor/coroutine.hh"
#include "utils/log.hh"

#include <exception>
#include <fcntl.h>
#include <system_error>
#include <span>
#include <utility>
#include <memory>

#include <fmt/std.h>

namespace corey {

namespace {

Log logger("io");

static_assert(
    sizeof(Promise<int>) == sizeof(io_uring_sqe::user_data),
    "Promise<int> must be the same size as io_uring data"
);

IoEngine* _instance = nullptr;

} // namespace


Future<> Context::cancel() {
    if (!this->_tag) {
        co_await std::make_exception_ptr(std::invalid_argument("empty context"));
    }
    auto ret = co_await IoEngine::instance().cancel(*this);
    if (ret < 0) {
        co_await std::make_exception_ptr(std::system_error(-ret, std::system_category(), "cancel failed"));
    }
    this->_tag = nullptr;
}

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
    _instance = this;
}

IoEngine::~IoEngine() {
    COREY_ASSERT(_pending == 0);
    io_uring_queue_exit(&_ring);
    _instance = nullptr;
}

Future<int> IoEngine::open(const char* path, int flags, Context* cc) {
    if ((flags & O_CREAT) || (flags & O_TMPFILE)) {
        return make_exception_future<int>(std::make_exception_ptr(std::invalid_argument("missing mode for open")));
    }
    return open(path, flags, 0, cc);
}

Future<int> IoEngine::open(const char* path, int flags, mode_t mode, Context* cc) {
    return prepare(io_uring_prep_openat,  cc, AT_FDCWD, path, flags, mode)->get_future();
}

Future<int> IoEngine::fsync(int fd, Context* cc) {
    return prepare(io_uring_prep_fsync, cc, fd, 0)->get_future();
}

Future<int> IoEngine::fdatasync(int fd, Context* cc) {
    return prepare(io_uring_prep_fsync, cc, fd, IORING_FSYNC_DATASYNC)->get_future();
}

Future<int> IoEngine::read(int fd, uint64_t offset, std::span<char> data, Context* cc) {
    return prepare(io_uring_prep_read, cc, fd, data.data(), data.size(), offset)->get_future();
}

Future<int> IoEngine::readv(int fd, uint64_t offset, std::span<iovec> iov, Context* cc) {
    return prepare(io_uring_prep_readv, cc, fd, iov.data(), iov.size(), offset)->get_future();
}

Future<int> IoEngine::writev(int fd, uint64_t offset, std::span<const iovec> iov, Context* cc) {
    return prepare(io_uring_prep_writev, cc, fd, iov.data(), iov.size(), offset)->get_future();
}

Future<int> IoEngine::write(int fd, uint64_t offset, std::span<const char> data, Context* cc) {
    return prepare(io_uring_prep_write, cc, fd, data.data(), data.size(), offset)->get_future();
}

Future<int> IoEngine::send(int fd, std::span<const char> buf, int flags, Context* cc) {
    return prepare(io_uring_prep_send, cc, fd, buf.data(), buf.size_bytes(), flags)->get_future();
}

Future<int> IoEngine::recv(int fd, std::span<char> buf, int flags, Context* cc) {
    return prepare(io_uring_prep_recv, cc, fd, buf.data(), buf.size_bytes(), flags)->get_future();
}

Future<int> IoEngine::close(int fd, Context* cc) {
    return prepare(io_uring_prep_close, cc, fd)->get_future();
}

Future<int> IoEngine::timeout(__kernel_timespec* ts, Context* cc) {
    return prepare(io_uring_prep_timeout, cc, ts, 0, IORING_TIMEOUT_ABS)->get_future();
}

Future<int> IoEngine::socket(int domain, int type, int protocol, Context* cc) {
    return prepare(io_uring_prep_socket, cc, domain, type, protocol, 0)->get_future();
}

Future<int> IoEngine::connect(int fd, const sockaddr* addr, socklen_t addrlen, Context* cc) {
    return prepare(io_uring_prep_connect, cc, fd, addr, addrlen)->get_future();
}

Future<int> IoEngine::accept(int fd, sockaddr* addr, socklen_t* addrlen, Context* cc) {
    return prepare(io_uring_prep_accept, cc, fd, addr, addrlen, 0)->get_future();
}

Future<int> IoEngine::setsockopt(int fd, int level, int optname, const void* optval, socklen_t optlen, Context*) {
    return posix_call(::setsockopt, fd, level, optname, optval, optlen);
}

Future<int> IoEngine::bind(int fd, const sockaddr* addr, socklen_t addrlen, Context*) {
    return posix_call(::bind, fd, addr, addrlen);
}

Future<int> IoEngine::listen(int fd, int backlog, Context*) {
    return posix_call(::listen, fd, backlog);
}

Future<int> IoEngine::cancel(const Context& cc) {
    return prepare(io_uring_prep_cancel, nullptr, cc._tag, 0)->get_future();
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

    auto check_queue = (!_reactor.has_progress() && (_inflight > 0))
        ? io_uring_wait_cqe
        : io_uring_peek_cqe;

    while (true) {
        io_uring_cqe *cqe;
        auto err = check_queue(&_ring, &cqe);
        if (err == -EAGAIN) {
            break;
        } else {
            check_queue = io_uring_peek_cqe;
        }
        COREY_ASSERT_MSG(err == 0, "io_uring_peek_cqe failed: {}", std::system_error(-err, std::system_category()));
        complete_cqe(*this, cqe);
    }
}

template<typename Func, typename... Args>
inline Promise<int>* IoEngine::prepare(Func&& func, Context* cc, Args&&... args) {
    if (auto sqe = io_uring_get_sqe(&_ring)) {
        std::invoke(std::forward<Func>(func), sqe, std::forward<Args>(args)...);
        auto comp = new (reinterpret_cast<void*>(&sqe->user_data)) Promise<int>;
        if (cc != nullptr) {
            cc->_tag = &sqe->user_data;
        }
        ++_pending;
        return comp;
    }
    panic("no sqe available in io_uring");
}

template<typename Func, typename... Args>
inline Future<int> IoEngine::posix_call(Func&& func, Args&&... args) {
    if (auto ret = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...); ret < 0) {
        return make_ready_future<int>(-errno);
    }
    return make_ready_future<int>(0);
}

} // namespace corey