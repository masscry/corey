#include "timer.hh"
#include "coroutine.hh"

#include <ctime>
#include <exception>
#include <linux/time_types.h>

namespace corey {

Future<> sleep(std::chrono::nanoseconds duration, Context& context) {
    timespec now {};
    clock_gettime(CLOCK_MONOTONIC, &now);
    __kernel_timespec ts {
        .tv_sec = now.tv_sec + duration.count() / 1'000'000'000,
        .tv_nsec = now.tv_nsec + duration.count() % 1'000'000'000
    };
    if (auto result = co_await IoEngine::instance().timeout(&ts, &context); result < 0) {
        if (result == -ETIME) {
            co_return;
        }
        co_await std::make_exception_ptr(
            std::system_error(-result, std::system_category(), "timeout failed")
        );
    }
}

Future<> sleep(std::chrono::nanoseconds duration) {
    Context ctx;
    return corey::sleep(duration, ctx);
}

} // namespace corey