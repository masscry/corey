#include "signal.hh"

#include "io.hh"
#include "liburing.h"
#include "reactor/io/file.hh"
#include "reactor/coroutine.hh"
#include "utils/log.hh"
#include "utils/common.hh"

#include <optional>
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

namespace corey {

Future<> handle_signals(int signum, SignalHandler handler) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signum);
    if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
        co_await std::make_exception_ptr(std::system_error(errno, std::system_category(), "Failed to block signal"));
    }

    auto fd = co_await IoEngine::instance().signalfd(invalid_fd, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (fd < 0) {
        co_await std::make_exception_ptr(std::system_error(-fd, std::system_category(), "Failed to create signalfd"));
    }

    auto waiter = std::make_optional<Promise<uint32_t>>();
    auto result = co_await IoEngine::instance().epoll_ctl(EPOLL_CTL_ADD, fd, EPOLLIN|EPOLLONESHOT, &waiter.value());
    if (result < 0) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "epoll_ctl failed"));
    }

    while (true) {
        signalfd_siginfo info{};
        co_await waiter->get_future();
        result = co_await IoEngine::instance().read(fd, 0, std::span<char>(reinterpret_cast<char*>(&info), sizeof(info)));
        if (result < 0) {
            co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "Failed to read signalfd"));
        }
        co_await handler(info.ssi_signo);

        //waiter = Promise<uint32_t>();
        co_await IoEngine::instance().epoll_ctl(EPOLL_CTL_MOD, fd, EPOLLIN|EPOLLONESHOT, &waiter.value());
    }
}

} // namespace corey
