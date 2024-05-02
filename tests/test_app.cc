#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "corey.hh"

class ApplicationTest : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    corey::Application app{0, nullptr};
};

TEST_F(ApplicationTest, Run) {
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        co_return 42;
    });
    EXPECT_EQ(result, 42);
}

TEST_F(ApplicationTest, RunWithException) {
    try {
        app.run([](const corey::ParseResult&) -> corey::Future<int> {
            throw std::runtime_error("Exception");
            co_return 0;
        });
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Exception");
    }
}

TEST_F(ApplicationTest, RunReadFromZero) {
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        auto file = co_await corey::File::open("/dev/zero", O_RDONLY);
        std::array<char, 100> data;
        co_await file.read(0, data);

        int sum = 0;
        for (auto c : data) {
            sum += c;
        }
        co_await std::move(file).close();
        co_return sum;
    });
    EXPECT_EQ(result, 0);
}

TEST_F(ApplicationTest, RunWriteToNull) {
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        auto file = co_await corey::File::open("/dev/null", O_WRONLY);
        std::array<char, 100> data;
        for (auto& c : data) {
            c = 42;
        }
        co_await file.write(0, data);
        co_await std::move(file).close();
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST_F(ApplicationTest, RunReadFromNonExistent) {
    try {
        app.run([](const corey::ParseResult&) -> corey::Future<int> {
            auto file = co_await corey::File::open("/nonexistent", O_RDONLY);
            std::array<char, 100> data;
            co_await file.read(0, data);
            co_await std::move(file).close();
            co_return 0;
        });
    } catch (const std::system_error& e) {
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST_F(ApplicationTest, RunWriteToReadOnly) {
    app.run([](const corey::ParseResult&) -> corey::Future<int> {
        auto file = co_await corey::File::open("/dev/zero", O_RDONLY);
        std::array<char, 100> data;
        for (auto& c : data) {
            c = 42;
        }
        try {
            co_await file.write(0, data);
        } catch (const std::system_error& e) {
            EXPECT_EQ(e.code().value(), EBADF);
        }
        co_await std::move(file).close();
        co_return 0;
    });
}
