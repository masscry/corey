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
        co_await file.close();
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
        co_await file.close();
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
            co_await file.close();
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
        co_await file.close();
        co_return 0;
    });
}

TEST_F(ApplicationTest, RunFileFsync) {
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        auto file = co_await corey::File::open("/dev/zero", O_RDONLY);
        try {
            co_await file.fsync();
        } catch (const std::system_error& e) {
            EXPECT_EQ(e.code().value(), EINVAL);
        }
        co_await file.close();
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST_F(ApplicationTest, RunFileFdatasync) {
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        auto file = co_await corey::File::open("/dev/zero", O_RDONLY);
        try {
            co_await file.fdatasync();
        } catch (const std::system_error& e) {
            EXPECT_EQ(e.code().value(), EINVAL);
        }
        co_await file.close();
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST_F(ApplicationTest, RunFileMoveAssignmentOperator) {
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        auto file = co_await corey::File::open("/dev/zero", O_RDONLY);
        corey::File file2;

        file2 = std::move(file);
        co_await file2.close();
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST_F(ApplicationTest, RunYield) {
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        co_await corey::yield();
        co_return 42;
    });
    EXPECT_EQ(result, 42);
}

TEST(Application, CheckAppInfo) {
    corey::Application app(0, nullptr, corey::ApplicationInfo{.name="test", .version= "1.0"});
    auto result = app.run([](const corey::ParseResult& opts) -> corey::Future<int> {
        EXPECT_EQ(opts.count("help"), 0);
        EXPECT_EQ(opts.count("version"), 0);
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckAppInfoHelp) {
    char* args[] = {
        const_cast<char*>("test"),
        const_cast<char*>("--help")
    };
    corey::Application app(
        std::extent_v<decltype(args)>,
        args,
        corey::ApplicationInfo{.name="test", .version="1.0"}
    );
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        ADD_FAILURE() << "This code should not be executed";
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckAppInfoVersion) {
    char* args[] = {
        const_cast<char*>("test"),
        const_cast<char*>("--version")
    };
    corey::Application app(
        std::extent_v<decltype(args)>,
        args,
        corey::ApplicationInfo{.name="test", .version="1.0"}
    );
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        ADD_FAILURE() << "This code should not be executed";
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckAppCustomOption) {
    char* args[] = {
        const_cast<char*>("test"),
        const_cast<char*>("--custom_opt=test")
    };
    corey::Application app(
        std::extent_v<decltype(args)>,
        args,
        corey::ApplicationInfo{.name="test", .version="1.0"}
    );

    app.add_options()("custom_opt", "Custom option", cxxopts::value<std::string>());

    auto result = app.run([](const corey::ParseResult& opts) -> corey::Future<int> {
        EXPECT_EQ(opts.count("custom_opt"), 1);
        EXPECT_EQ(opts["custom_opt"].as<std::string>(), "test");
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckAppCustomOptionWithDefault) {
    char* args[] = {
        const_cast<char*>("test")
    };
    corey::Application app(
        std::extent_v<decltype(args)>,
        args,
        corey::ApplicationInfo{.name="test", .version="1.0"}
    );

    app.add_options()("custom_opt", "Custom option", cxxopts::value<std::string>()->default_value("default"));

    auto result = app.run([](const corey::ParseResult& opts) -> corey::Future<int> {
        EXPECT_EQ(opts.count("custom_opt"), 0);
        EXPECT_EQ(opts["custom_opt"].as<std::string>(), "default");
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckAppCustomIntOption) {
    char* args[] = {
        const_cast<char*>("test"),
        const_cast<char*>("--custom_int_opt=42")
    };
    corey::Application app(
        std::extent_v<decltype(args)>,
        args,
        corey::ApplicationInfo{.name="test", .version="1.0"}
    );

    app.add_options()("custom_int_opt", "Custom int option", cxxopts::value<int>());

    auto result = app.run([](const corey::ParseResult& opts) -> corey::Future<int> {
        EXPECT_EQ(opts.count("custom_int_opt"), 1);
        EXPECT_EQ(opts["custom_int_opt"].as<int>(), 42);
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckAppCustomIntOptionWithDefault) {
    char* args[] = {
        const_cast<char*>("test")
    };
    corey::Application app(
        std::extent_v<decltype(args)>,
        args,
        corey::ApplicationInfo{.name="test", .version="1.0"}
    );

    app.add_options()("custom_int_opt", "Custom int option", cxxopts::value<int>()->default_value("42"));

    auto result = app.run([](const corey::ParseResult& opts) -> corey::Future<int> {
        EXPECT_EQ(opts.count("custom_int_opt"), 0);
        EXPECT_EQ(opts["custom_int_opt"].as<int>(), 42);
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckAppPositional) {
    char* args[] = {
        const_cast<char*>("test"),
        const_cast<char*>("positional")
    };
    corey::Application app(
        std::extent_v<decltype(args)>,
        args,
        corey::ApplicationInfo{.name="test", .version="1.0"}
    );

    app.add_options()("positional", "Positional argument", cxxopts::value<std::string>());

    app.add_positional_options("positional");

    auto result = app.run([](const corey::ParseResult& opts) -> corey::Future<int> {
        EXPECT_EQ(opts.count("positional"), 1);
        EXPECT_EQ(opts["positional"].as<std::string>(), "positional");
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckAppPositionalWithDefault) {
    char* args[] = {
        const_cast<char*>("test")
    };
    corey::Application app(
        std::extent_v<decltype(args)>,
        args,
        corey::ApplicationInfo{.name="test", .version="1.0"}
    );

    app.add_options()("positional", "Positional argument", cxxopts::value<std::string>()->default_value("default"));

    app.add_positional_options("positional");

    auto result = app.run([](const corey::ParseResult& opts) -> corey::Future<int> {
        EXPECT_EQ(opts.count("positional"), 0);
        EXPECT_EQ(opts["positional"].as<std::string>(), "default");
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckSleepTimeout) {
    using namespace std::chrono_literals;

    corey::Application app(0, nullptr);
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        auto start = std::chrono::steady_clock::now();
        co_await corey::sleep(1s);
        auto end = std::chrono::steady_clock::now();
        EXPECT_GE(end - start, 1s);
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckSleepZeroTimeout) {
    using namespace std::chrono_literals;

    corey::Application app(0, nullptr);
    auto result = app.run([](const corey::ParseResult&) -> corey::Future<int> {
        auto start = std::chrono::steady_clock::now();
        co_await corey::sleep(0s);
        auto end = std::chrono::steady_clock::now();
        EXPECT_LE(end - start, 1ms);
        co_return 0;
    });
    EXPECT_EQ(result, 0);
}

TEST(Application, CheckSleepNegativeTimeout) {
    using namespace std::chrono_literals;

    corey::Application app(0, nullptr);
    try {
        app.run([](const corey::ParseResult&) -> corey::Future<int> {
            co_await corey::sleep(-1s);
            co_return 0;
        });
    } catch (const std::system_error& e) {
        EXPECT_EQ(e.code().value(), EINVAL);
    }
}
