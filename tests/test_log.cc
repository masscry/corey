#include "defer.hh"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <corey.hh>

#include <fmt/format.h>

#include <type_traits>
#include <sstream>
#include <regex>

class MockConsole {
public:
    MOCK_METHOD(int, write, (std::span<const char> message), (const));
};

class LogTest : public testing::Test {
protected:

    LogTest() {
        using ::testing::_;
        auto old_sink = corey::Log::set_default_sink(corey::Sink::make<MockConsole>());
        restore_old_sink = corey::Defer([old_sink = std::move(old_sink)]() noexcept {
            corey::Log::set_default_sink(old_sink);
        });

        console = &corey::Log::get_default_sink().as<MockConsole>();
        logStream = std::stringstream();
        ON_CALL(*console, write(_)).WillByDefault([this](std::span<const char> message) {
            fmt::print("{}", fmt::join(message, ""));
            logStream.write(message.data(), message.size());
            return message.size();
        });
    }

    ~LogTest() override = default;

    corey::Defer<> restore_old_sink;
    std::stringstream logStream;
    const MockConsole* console = nullptr;
};

TEST_F(LogTest, MockConsoleSink) {
    using ::testing::_;

    std::stringstream logStream;
    corey::Log test("test");
    
    ON_CALL(*console, write(_)).WillByDefault([&logStream](std::span<const char> message) {
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

    EXPECT_CALL(*console, write(_)).Times(std::extent<decltype(cases)>{});
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

TEST_F(LogTest, LogLevelFiltering) {
    using ::testing::_;

    corey::Log test("test");
    test.set_level(corey::Log::Level::warn);

    EXPECT_CALL(*console, write(_)).Times(0);
    test.info("This message should not be printed");
    test.debug("This message should not be printed");

    EXPECT_CALL(*console, write(_)).Times(2);
    test.warn("This is a warning message");
    test.error("This is an error message");

    test.set_level(corey::Log::Level::info);
    EXPECT_CALL(*console, write(_)).Times(3);
    test.info("This is an info message");
    test.warn("This is a warning message");
    test.error("This is an error message");
}

TEST_F(LogTest, Simple) {
    using ::testing::_;

    corey::Log test("test");
    EXPECT_CALL(*console, write(_)).Times(1);
    test.info("hello!");
}

TEST_F(LogTest, LogLevelTextFormatting) {
    auto debug = fmt::format("{}", corey::Log::Level::debug);
    EXPECT_EQ(debug, "debug");

    auto info = fmt::format("{}", corey::Log::Level::info);
    EXPECT_EQ(info, "info");

    auto warn = fmt::format("{}", corey::Log::Level::warn);
    EXPECT_EQ(warn, "warn");

    auto error = fmt::format("{}", corey::Log::Level::error);
    EXPECT_EQ(error, "error");
}

TEST_F(LogTest, LogConstructor) {
    corey::Log test("test");
    EXPECT_EQ(test.get_name(), "test");
    EXPECT_EQ(test.get_level(), corey::Log::Level::info);

    corey::Log test1("test1", corey::Log::Level::warn);
    EXPECT_EQ(test1.get_name(), "test1");
    EXPECT_EQ(test1.get_level(), corey::Log::Level::warn);

    corey::Log test2("test2", corey::Sink::make<MockConsole>());
    EXPECT_EQ(test2.get_name(), "test2");
    EXPECT_EQ(test2.get_level(), corey::Log::Level::info);

    corey::Log test3("test3", corey::Log::Level::error, corey::Sink::make<MockConsole>());
    EXPECT_EQ(test3.get_name(), "test3");
    EXPECT_EQ(test3.get_level(), corey::Log::Level::error);
}

TEST_F(LogTest, Assert) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(0);

    EXPECT_DEATH({
        EXPECT_CALL(*console, write(_)).Times(1);
        corey::panic("at the disco");
    }, ".*");
    EXPECT_DEATH({
        EXPECT_CALL(*console, write(_)).Times(1);
        COREY_ASSERT(false);
    }, ".*");
}

TEST_F(LogTest, Task) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(1);

    auto task = corey::make_task([]() {
        corey::Log log("task");
        log.info("hello!");
    });
    task();
}

TEST_F(LogTest, EmptyName) {
    EXPECT_DEATH({
        using ::testing::_;
        EXPECT_CALL(*console, write(_)).Times(1);
        corey::Log test("");
    }, ".*");
}

TEST_F(LogTest, MultipleLogs) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(3);

    corey::Log test("test");
    test.info("hello!");
    test.warn("world!");
    test.error("error!");
}

TEST_F(LogTest, LogLevels) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(3);

    corey::Log test("test", corey::Log::Level::info);
    test.info("info message");
    test.warn("warning message");
    test.error("error message");
    test.debug("debug message");
}

TEST_F(LogTest, LogWithArgs) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(3);

    corey::Log test("test");
    test.info("Hello, {}", "world");
    test.warn("The answer is {}", 42);
    test.error("Something went wrong: {}", "unknown error");
}

TEST_F(LogTest, LogWithException) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(1);

    corey::Log test("test");
    try {
        throw std::runtime_error("Exception occurred");
    } catch (const std::exception& e) {
        test.error("Exception: {}", e.what());
    }
}

TEST_F(LogTest, LogApplicationOrphanedException) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(testing::AtLeast(1));

    corey::Log test_log("test");
    corey::Application app(0, nullptr);

    auto result = app.run([](const corey::ParseResult&, corey::Log& test_log) -> corey::Future<int> {

        corey::Promise<> good_promise, bad_promise;

        auto good_fut = good_promise.get_future();
        auto bad_fut = bad_promise.get_future();

        std::ignore = [](auto& log, auto good, auto bad) mutable -> corey::Future<>{
            log.info("Good promise");
            good.set();
            log.info("yield");
            co_await corey::yield();
            log.info("throwing exception");
            throw std::runtime_error("Orphaned exception");
            log.info("Bad promise");
            bad.set();
        }(test_log, std::move(good_promise), std::move(bad_promise));

        test_log.info("wait good");
        EXPECT_NO_THROW(co_await std::move(good_fut));

        test_log.info("wait bad");
        EXPECT_THROW(co_await std::move(bad_fut), corey::BrokenPromise);

        test_log.info("done");
        co_return 0;
    }, test_log);

    auto output = logStream.str();
    EXPECT_THAT(output, testing::HasSubstr("orphan: unhandled exception: Orphaned exception"));
    EXPECT_EQ(result, 0);
}

TEST_F(LogTest, LogApplicationOrphanedExceptionWithReturnValue) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(testing::AtLeast(1));

    corey::Log test_log("test");
    corey::Application app(0, nullptr);
    auto result = app.run([](const corey::ParseResult&, corey::Log& test_log) -> corey::Future<int> {

        corey::Promise<> good_promise, bad_promise;

        auto good_fut = good_promise.get_future();
        auto bad_fut = bad_promise.get_future();

        std::ignore = [](auto& log, auto good, auto bad) mutable -> corey::Future<int>{
            log.info("Good promise");
            good.set();
            log.info("yield");
            co_await corey::yield();
            log.info("throwing exception");
            co_return std::make_exception_ptr(std::runtime_error("Orphaned exception"));
            log.info("Bad promise");
            bad.set();
        }(test_log, std::move(good_promise), std::move(bad_promise));

        test_log.info("wait good");
        EXPECT_NO_THROW(co_await std::move(good_fut));

        test_log.info("wait bad");
        EXPECT_THROW(co_await std::move(bad_fut), corey::BrokenPromise);

        test_log.info("done");
        co_return 0;
    }, test_log);

    auto output = logStream.str();
    EXPECT_THAT(output, testing::HasSubstr("orphan: unhandled exception: Orphaned exception"));
    EXPECT_EQ(result, 0);
}


TEST_F(LogTest, LogApplicationOrphanedExceptionWithAwaitException) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(testing::AtLeast(1));

    corey::Log test_log("test");
    corey::Application app(0, nullptr);
    auto result = app.run([](const corey::ParseResult&, corey::Log& test_log) -> corey::Future<int> {

        corey::Promise<> good_promise, bad_promise;

        auto good_fut = good_promise.get_future();
        auto bad_fut = bad_promise.get_future();

        std::ignore = [](auto& log, auto good, auto bad) mutable -> corey::Future<>{
            log.info("Good promise");
            good.set();
            co_await corey::yield();
            log.info("throwing exception");
            co_await std::make_exception_ptr(std::runtime_error("Orphaned exception"));
            log.info("Bad promise");
            bad.set();
        }(test_log, std::move(good_promise), std::move(bad_promise));

        test_log.info("wait good");
        EXPECT_NO_THROW(co_await std::move(good_fut));

        test_log.info("wait bad");
        EXPECT_THROW(co_await std::move(bad_fut), corey::BrokenPromise);

        test_log.info("done");

        co_return 0;
    }, test_log);

    auto output = logStream.str();
    EXPECT_THAT(output, testing::HasSubstr("orphan: unhandled exception: Orphaned exception"));
    EXPECT_EQ(result, 0);
}

TEST_F(LogTest, LogSetDefaultSink) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(0);

    auto new_sink = corey::Sink::make<MockConsole>();

    corey::Log::set_default_sink(new_sink);
    corey::Log test("test");

    EXPECT_EQ(test.get_sink(), new_sink);
}

TEST_F(LogTest, LogSetDefaultLevel) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(0);

    corey::Log::set_default_level(corey::Log::Level::warn);
    corey::Log test("test");

    EXPECT_EQ(test.get_level(), corey::Log::Level::warn);
}

TEST_F(LogTest, LogSetLevel) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(0);

    corey::Log test("test");
    test.set_level(corey::Log::Level::warn);

    EXPECT_EQ(test.get_level(), corey::Log::Level::warn);
}

TEST_F(LogTest, LogSetSink) {
    using ::testing::_;
    EXPECT_CALL(*console, write(_)).Times(0);

    auto new_sink = corey::Sink::make<MockConsole>();

    corey::Log test("test");
    test.set_sink(new_sink);

    EXPECT_EQ(test.get_sink(), new_sink);
}