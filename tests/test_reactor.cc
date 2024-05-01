#include "future.hh"
#include "reactor/reactor.hh"
#include "reactor/io/io.hh"
#include "task.hh"

#include <vector>
#include <memory>

#include <gtest/gtest.h>

TEST(Reactor, ReactorAdd) {
    corey::Promise<int> test;
    corey::Reactor reactor;

    auto fut = test.get_future();

    EXPECT_NO_THROW({
        reactor.add(corey::make_task([test = std::move(test)]() mutable {
            test.set(101);
        }));
    });

    EXPECT_NO_THROW({
        reactor.run();
    });

    EXPECT_EQ(fut.get(), 101);
}

TEST(Reactor, IoEngineRead) {
    corey::Reactor reactor;
    corey::IoEngine io(reactor);

    int fd = open("/dev/zero", O_RDONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf;
    auto fut = io.read(fd, 0, buf);

    reactor.run();

    EXPECT_EQ(fut.get(), 1024);
    close(fd);
}

TEST(Reactor, IoEngineWrite) {
    corey::Reactor reactor;
    corey::IoEngine io(reactor);

    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf;
    auto fut = io.write(fd, 0, buf);

    reactor.run();

    EXPECT_EQ(fut.get(), 1024);
    close(fd);
}

TEST(Reactor, IoEngineClose) {
    corey::Reactor reactor;
    corey::IoEngine io(reactor);

    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    auto fut = io.close(fd);

    reactor.run();

    EXPECT_EQ(fut.get(), 0);
}

TEST(Reactor, IoEngineMultipleRead) {
    corey::Reactor reactor;
    corey::IoEngine io(reactor);

    int fd = open("/dev/zero", O_RDONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf;
    auto fut1 = io.read(fd, 0, buf);
    auto fut2 = io.read(fd, 0, buf);
    auto fut3 = io.read(fd, 0, buf);

    reactor.run();

    EXPECT_EQ(fut1.get(), 1024);
    EXPECT_EQ(fut2.get(), 1024);
    EXPECT_EQ(fut3.get(), 1024);
    close(fd);
}

TEST(Reactor, IoEngineMultipleWrite) {
    corey::Reactor reactor;
    corey::IoEngine io(reactor);

    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf;
    auto fut1 = io.write(fd, 0, buf);
    auto fut2 = io.write(fd, 0, buf);
    auto fut3 = io.write(fd, 0, buf);

    reactor.run();

    EXPECT_EQ(fut1.get(), 1024);
    EXPECT_EQ(fut2.get(), 1024);
    EXPECT_EQ(fut3.get(), 1024);
    close(fd);
}


