#include "socket.hh"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <corey.hh>
#include <system_error>


class SocketTest : public testing::Test {
protected:
    void SetUp() override {
        app.emplace(0, nullptr, corey::ApplicationInfo{
            "test",
            "test app"
        });
    }

    void TearDown() override {
        app.reset();       
    }

    std::optional<corey::Application> app;
};

constexpr auto TEST_SOCK = 55305;

TEST_F(SocketTest, TestSocketIO) {

    auto result = app->run([](const auto&) -> corey::Future<int> {

        auto listener = co_await corey::Socket::make_tcp_listener(TEST_SOCK);
        std::exception_ptr eptr;

        auto client_fib = []() -> corey::Future<> {
            EXPECT_NO_THROW(
                auto client = co_await corey::Socket::make_tcp_connect("127.0.0.1", TEST_SOCK);
                std::string message = "Hello, World!";
                auto size = co_await client.write(std::span(message));
                EXPECT_EQ(size, message.size());
                co_await client.close();
            );
        }();

        EXPECT_NO_THROW({
            constexpr auto buf_size = 1024;
            auto client_sock = co_await listener.accept();
            char buffer[buf_size];
            auto size = co_await client_sock.read(std::span(buffer, buf_size));
            EXPECT_EQ(size, 13);
            EXPECT_EQ(std::string(buffer, size), "Hello, World!");
            co_await client_sock.close();
            co_await listener.close();
        });

        co_await std::move(client_fib);
        co_return 0;
    });

    EXPECT_EQ(result, 0);
}

TEST_F(SocketTest, TestSocketConstructors) {
    auto result = app->run([](const auto&) -> corey::Future<int> {

        EXPECT_NO_THROW({
            corey::Socket sock;
            EXPECT_EQ(sock.fd(), corey::invalid_fd);
        });

        EXPECT_NO_THROW({
            auto tcp_listener = co_await corey::Socket::make_tcp_listener(TEST_SOCK);
            EXPECT_NE(tcp_listener.socket().fd(), corey::invalid_fd);
            auto old_fd = tcp_listener.socket().fd();

            corey::Server moved;
            
            moved = std::move(tcp_listener);

            EXPECT_EQ(tcp_listener.socket().fd(), corey::invalid_fd);
            EXPECT_EQ(moved.socket().fd(), old_fd);

            co_await moved.close();
        });

        co_return 0;

    });
    EXPECT_EQ(result, 0);
}

TEST_F(SocketTest, TestSocketPanic) {
    GTEST_FLAG_SET(death_test_style, "threadsafe");

    auto result = app->run([](const auto&) -> corey::Future<int> {

        EXPECT_THROW({
            corey::Socket sock;
            co_await sock.close();
        }, std::system_error);

        EXPECT_DEATH({
            auto tcp_listener = co_await corey::Socket::make_tcp_listener(TEST_SOCK);
            EXPECT_NE(tcp_listener.socket().fd(), corey::invalid_fd);
        }, ".*");

        co_return 0;
    });
    EXPECT_EQ(result, 0);
}