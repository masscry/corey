
#include <gtest/gtest.h>
#include <csignal>

#include "corey.hh"
#include "reactor/signal.hh"
#include "reactor/future.hh"
#include "reactor/coroutine.hh"

// Test case: Register a signal handler for SIGUSR2 and verify if it is called
TEST(HandleSignalsTest, RegisterSignalHandlerForSIGUSR2) {
    corey::Application app(0, nullptr);
    auto result = app.run([](corey::ParseResult&) -> corey::Future<int> {
        using namespace std::chrono_literals;

        bool signalHandled = false;

        // Register a signal handler for SIGUSR2
        std::ignore = corey::handle_signals(SIGUSR2, corey::make_signal_handler(
            [&](int signum) -> corey::Future<> {
                EXPECT_EQ(signum, SIGUSR2);
                signalHandled = true;
                co_return;
            }
        ));

        // Send SIGUSR2 signal
        std::raise(SIGUSR2);
        co_await corey::sleep(10s);

        // Verify if the signal handler was called
        EXPECT_TRUE(signalHandled);
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}
