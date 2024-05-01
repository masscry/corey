#include "io.hh"

#include "common/macro.hh"
#include "future.hh"
#include "liburing/io_uring.h"
#include "reactor/reactor.hh"
#include "reactor/task.hh"
#include "utils/log.hh"
#include "utils/common.hh"

#include <system_error>
#include <span>
#include <utility>
#include <memory>

#include <fmt/std.h>

#include <liburing.h>

namespace corey {

namespace {

Log logger("io");

static_assert(
    sizeof(Promise<int>) == sizeof(io_uring_sqe::user_data),
    "Promise<int> must be the same size as io_uring data"
);

} // namespace

IoEngine::IoEngine(Reactor& reactor) {
    if (int ret = io_uring_queue_init(max_events, &_ring, 0); ret != 0) {
        throw std::system_error(-ret, std::system_category(), "io_uring_queue_init failed");
    }
    _poll_routine = reactor.add(make_routine([this] {
        submit_pending();
        complete_ready();
    }));
}

IoEngine::~IoEngine() {
    COREY_ASSERT(_pending == 0);
    io_uring_queue_exit(&_ring);
}

Future<int> IoEngine::read(int fd, uint64_t offset, std::span<char> data) {
    return prepare(io_uring_prep_read, fd, data.data(), data.size(), offset)->get_future();
}

Future<int> IoEngine::write(int fd, uint64_t offset, std::span<const char> data) {
    return prepare(io_uring_prep_write, fd, data.data(), data.size(), offset)->get_future();
}

Future<int> IoEngine::close(int fd) {
    return prepare(io_uring_prep_close, fd)->get_future();
}

void IoEngine::submit_pending() {
    while(_pending > 0) {
        int ret = io_uring_submit(&_ring);
        if (ret < 0) {
            logger.error("io_uring_submit failed: {}", std::system_error(-ret, std::system_category()));
            return;
        }
        _pending -= ret;
    }
}

void IoEngine::complete_ready() {
    io_uring_cqe *cqe;
    while (io_uring_peek_cqe(&_ring, &cqe) == 0) {
        auto comp = reinterpret_cast<Promise<int>*>(&cqe->user_data);
        comp->set(cqe->res);
        comp->~Promise();
        io_uring_cqe_seen(&_ring, cqe);
    }
}

template<typename Func, typename... Args>
inline
Promise<int>* IoEngine::prepare(Func func, Args&&... args) {
    if (auto sqe = io_uring_get_sqe(&_ring)) {
        std::invoke(func, sqe, std::forward<Args>(args)...);
        auto comp = new (reinterpret_cast<void*>(&sqe->user_data)) Promise<int>;
        ++_pending;
        return comp;
    }
    panic("no sqe available in io_uring");
}

} // namespace corey