#include "common.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "reactor/task.hh"
#include "common/defer.hh"
#include "common/sink.hh"
#include "utils/log.hh"

#include <type_traits>

#include <sstream>
#include <regex>

class MockConsole {
public:
    MOCK_METHOD(int, write, (std::span<char> message), (const));
};

TEST(Log, MockConsoleSink) {
    using ::testing::_;

    std::stringstream logStream;
    corey::Log test("test", corey::Sink<void>::make<MockConsole>());
    auto& console = test.get_sink().as<MockConsole>().get_impl();
    
    ON_CALL(console, write(_)).WillByDefault([&logStream](std::span<char> message) {
        logStream.write(message.data(), message.size());
        return message.size();
    });

    struct Case {
        corey::Log::Level level;
        std::string message;
    } cases [] = {
        { corey::Log::Level::info, "info message" },
        { corey::Log::Level::warn, "warning message" },
        { corey::Log::Level::error, "error message" },
    };

    EXPECT_CALL(console, write(_)).Times(std::extent<decltype(cases)>{});
    std::ranges::for_each(cases, [&](const auto& testCase) {
        test.log(testCase.level, "{}", testCase.message);
    });

    std::string line;
    std::regex logRegex(R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{9}\]\[(\w+)\] (.*): (.*))"); // Modify the regular expression pattern

    auto span = std::span<Case>(cases);
    while (std::getline(logStream, line)) {
        std::smatch match;

        ASSERT_TRUE(std::regex_search(line, match, logRegex));
        ASSERT_EQ(match.size(), 4);
        ASSERT_EQ(match[1], fmt::format("{}", span.front().level));
        ASSERT_EQ(match[2], "test");
        ASSERT_EQ(match[3], span.front().message);
        span = span.subspan(1);
    }
}

TEST(Log, LogLevelFiltering) {
    using ::testing::_;

    corey::Log test("test", corey::Sink<void>::make<MockConsole>());
    test.set_level(corey::Log::Level::warn);

    EXPECT_CALL(test.get_sink().as<MockConsole>().get_impl(), write(_)).Times(0);
    test.info("This message should not be printed");
    test.debug("This message should not be printed");

    EXPECT_CALL(test.get_sink().as<MockConsole>().get_impl(), write(_)).Times(2);
    test.warn("This is a warning message");
    test.error("This is an error message");

    test.set_level(corey::Log::Level::info);
    EXPECT_CALL(test.get_sink().as<MockConsole>().get_impl(), write(_)).Times(3);
    test.info("This is an info message");
    test.warn("This is a warning message");
    test.error("This is an error message");
}

TEST(Log, Simple) {
    corey::Log test("test");
    test.info("hello!");
}

TEST(Log, Assert) {
    EXPECT_DEATH({ corey::panic("at the disco"); }, ".*");
    EXPECT_DEATH({ std::ignore = COREY_ASSERT(false); }, ".*");
    EXPECT_TRUE(COREY_ASSERT(true));
}

TEST(Log, Task) {
    auto task = corey::make_task([]() {
        corey::Log log("task");
        log.info("hello!");
    });
    task.execute_once();
}

TEST(Log, EmptyName) {
    EXPECT_DEATH({ corey::Log test(""); }, ".*");
}

TEST(Log, MultipleLogs) {
    corey::Log test("test");
    test.info("hello!");
    test.warn("world!");
    test.error("error!");
}

TEST(Log, LogLevels) {
    corey::Log test("test");
    test.info("info message");
    test.warn("warning message");
    test.error("error message");
    test.debug("debug message");
}

TEST(Log, LogWithArgs) {
    corey::Log test("test");
    test.info("Hello, {}", "world");
    test.warn("The answer is {}", 42);
    test.error("Something went wrong: {}", "unknown error");
}

TEST(Log, LogWithException) {
    corey::Log test("test");
    try {
        throw std::runtime_error("Exception occurred");
    } catch (const std::exception& e) {
        test.error("Exception: {}", e.what());
    }
}

TEST(Defer, SingleDefer) {
    bool executed = false;
    {
        auto def = corey::defer([&]() noexcept {
            executed = true;
        });
    }
    EXPECT_TRUE(executed);
}

TEST(Defer, DeferWithCancel) {
    bool executed = false;
    {
        auto def = corey::defer([&]() noexcept {
            executed = true;
        });
        def.cancel();
    }
    EXPECT_FALSE(executed);
}

TEST(Defer, MultipleDefers) {
    std::vector<int> values;
    {
        auto def = corey::defer([&]() noexcept {
            values.push_back(1);
        });
        auto def1 = corey::defer([&]() noexcept {
            values.push_back(2);
        });
        auto def2 = corey::defer([&]() noexcept{
            values.push_back(3);
        });
    }
    EXPECT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], 3);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 1);
}

TEST(Defer, MultipleDefersWithCancel) {
    std::vector<int> values;
    {
        auto def = corey::defer([&]() noexcept {
            values.push_back(1);
        });
        auto def1 = corey::defer([&]() noexcept {
            values.push_back(2);
        });
        auto def2 = corey::defer([&]() noexcept{
            values.push_back(3);
        });
        def1.cancel();
    }
    EXPECT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], 3);
    EXPECT_EQ(values[1], 1);
}

TEST(Defer, MultipleDefersWithCancelAll) {
    std::vector<int> values;
    {
        auto def = corey::defer([&]() noexcept {
            values.push_back(1);
        });
        auto def1 = corey::defer([&]() noexcept {
            values.push_back(2);
        });
        auto def2 = corey::defer([&]() noexcept{
            values.push_back(3);
        });
        def.cancel();
        def1.cancel();
        def2.cancel();
    }
    EXPECT_TRUE(values.empty());
}

TEST(Defer, DeferWithException) {
    bool executed = false;
    try {
        auto def = corey::defer([&]() noexcept {
            executed = true;
        });
        throw std::runtime_error("Exception occurred");
    } catch (const std::exception& e) {}
    EXPECT_TRUE(executed);
}

TEST(Defer, DeferWithExceptionAndCancel) {
    bool executed = false;
    try {
        auto def = corey::defer([&]() noexcept {
            executed = true;
        });
        def.cancel();
        throw std::runtime_error("Exception occurred");
    } catch (const std::exception& e) {}
    EXPECT_FALSE(executed);
}

TEST(Defer, DeferWithExceptionAndCancelMultiple) {
    std::vector<int> values;
    try {
        auto def = corey::defer([&]() noexcept {
            values.push_back(1);
        });
        auto def1 = corey::defer([&]() noexcept {
            values.push_back(2);
        });
        auto def2 = corey::defer([&]() noexcept{
            values.push_back(3);
        });
        def1.cancel();
        throw std::runtime_error("Exception occurred");
    } catch (const std::exception& e) {}
    EXPECT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], 3);
    EXPECT_EQ(values[1], 1);
}

TEST(Defer, DeferWithExceptionAndCancelAll) {
    std::vector<int> values;
    try {
        auto def = corey::defer([&]() noexcept {
            values.push_back(1);
        });
        auto def1 = corey::defer([&]() noexcept {
            values.push_back(2);
        });
        auto def2 = corey::defer([&]() noexcept{
            values.push_back(3);
        });
        def.cancel();
        def1.cancel();
        def2.cancel();
        throw std::runtime_error("Exception occurred");
    } catch (const std::exception& e) {}
    EXPECT_TRUE(values.empty());
}

TEST(Defer, ReturnDeferFromFuncTypeErased) {
    bool executed = false;
    {
        corey::Defer<> erased;
        {
            auto def = [&executed]() -> corey::Defer<> {
                return corey::defer([&executed]() noexcept {
                    executed = true;
                });
            }();

            EXPECT_FALSE(executed);
            erased = std::move(def);
        }

        EXPECT_FALSE(executed);
    }
    EXPECT_TRUE(executed);
}

TEST(Defer, ReturnDeferFromFuncTypeErasedWithCancel) {
    bool executed = false;
    {
        corey::Defer<> erased;
        {
            auto def = [&executed]() -> corey::Defer<> {
                return corey::defer([&executed]() noexcept {
                    executed = true;
                });
            }();

            EXPECT_FALSE(executed);
            erased = std::move(def);
            erased.cancel();
        }

        EXPECT_FALSE(executed);
    }
    EXPECT_FALSE(executed);
}

TEST(Defer, ReturnDeferFromFuncTypeErasedWithException) {
    bool executed = false;
    try {
        corey::Defer<> erased;
        {
            auto def = [&executed]() -> corey::Defer<> {
                return corey::defer([&executed]() noexcept {
                    executed = true;
                });
            }();

            EXPECT_FALSE(executed);
            erased = std::move(def);
            throw std::runtime_error("Exception occurred");
        }
    } catch (const std::exception& e) {}
    EXPECT_TRUE(executed);
}

TEST(Defer, ReturnDeferFromFuncTypeErasedWithExceptionAndCancel) {
    bool executed = false;
    try {
        corey::Defer<> erased;
        {
            auto def = [&executed]() -> corey::Defer<> {
                return corey::defer([&executed]() noexcept {
                    executed = true;
                });
            }();

            EXPECT_FALSE(executed);
            erased = std::move(def);
            erased.cancel();
            throw std::runtime_error("Exception occurred");
        }
    } catch (const std::exception& e) {}
    EXPECT_FALSE(executed);
}

TEST(Defer, ReturnDeferFromFuncTypeErasedWithExceptionAndCancelMultiple) {
    std::vector<int> values;
    try {
        corey::Defer<> erased;
        {
            auto def = [&values]() -> corey::Defer<> {
                return corey::defer([&values]() noexcept {
                    values.push_back(1);
                });
            }();

            auto def1 = [&values]() -> corey::Defer<> {
                return corey::defer([&values]() noexcept {
                    values.push_back(2);
                });
            }();

            auto def2 = [&values]() -> corey::Defer<> {
                return corey::defer([&values]() noexcept {
                    values.push_back(3);
                });
            }();

            def1.cancel();
            erased = std::move(def);
            erased.cancel();
            throw std::runtime_error("Exception occurred");
        }
    } catch (const std::exception& e) {}
    ASSERT_EQ(values.size(), 1);
    EXPECT_EQ(values[0], 3);
}
