#include "reactor/future.hh"
#include "reactor/reactor.hh"
#include "reactor/io/io.hh"
#include "reactor/task.hh"

#include <cerrno>
#include <vector>
#include <memory>

#include <gtest/gtest.h>

TEST(ReactorTest, ReactorAdd) {
    corey::Promise<int> test;
    corey::Reactor reactor;

    auto fut = test.get_future();

    EXPECT_NO_THROW({
        reactor.add_task(corey::make_task([test = std::move(test)]() mutable {
            test.set(101);
        }));
    });

    EXPECT_NO_THROW({
        reactor.run();
    });

    EXPECT_EQ(fut.get(), 101);
}

class ReactorIOTest : public testing::Test {
protected:
    void SetUp() override {
        _reactor = std::make_unique<corey::Reactor>();
        _io = std::make_unique<corey::IoEngine>(*_reactor);
    }

    void TearDown() override {
        _io.reset();
        _reactor.reset();
    }

    std::unique_ptr<corey::Reactor> _reactor;
    std::unique_ptr<corey::IoEngine> _io;
};



TEST_F(ReactorIOTest, IoEngineRead) {
    int fd = open("/dev/zero", O_RDONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf;
    auto fut = _io->read(fd, 0, buf);

    _reactor->run();

    EXPECT_EQ(fut.get(), 1024);
    close(fd);
}

TEST_F(ReactorIOTest, IoEngineWrite) {
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf;
    auto fut = _io->write(fd, 0, buf);

    _reactor->run();

    EXPECT_EQ(fut.get(), 1024);
    close(fd);
}

TEST_F(ReactorIOTest, IoEngineClose) {
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    auto fut = _io->close(fd);

    _reactor->run();

    EXPECT_EQ(fut.get(), 0);
}

TEST_F(ReactorIOTest, IoEngineMultipleRead) {
    int fd = open("/dev/zero", O_RDONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf;
    auto fut1 = _io->read(fd, 0, buf);
    auto fut2 = _io->read(fd, 0, buf);
    auto fut3 = _io->read(fd, 0, buf);

    _reactor->run();

    EXPECT_EQ(fut1.get(), 1024);
    EXPECT_EQ(fut2.get(), 1024);
    EXPECT_EQ(fut3.get(), 1024);
    close(fd);
}

TEST_F(ReactorIOTest, IoEngineMultipleWrite) {
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf;
    auto fut1 = _io->write(fd, 0, buf);
    auto fut2 = _io->write(fd, 0, buf);
    auto fut3 = _io->write(fd, 0, buf);

    _reactor->run();

    EXPECT_EQ(fut1.get(), 1024);
    EXPECT_EQ(fut2.get(), 1024);
    EXPECT_EQ(fut3.get(), 1024);
    close(fd);
}

TEST_F(ReactorIOTest, IoEngineMultipleClose) {
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    auto fut1 = _io->close(fd);
    auto fut2 = _io->close(fd);
    auto fut3 = _io->close(fd);

    _reactor->run();

    EXPECT_EQ(fut1.get(), 0);
    EXPECT_EQ(fut2.get(), -EBADF);
    EXPECT_EQ(fut3.get(), -EBADF);
}

TEST_F(ReactorIOTest, IoEngineReadv) {
    int fd = open("/dev/zero", O_RDONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf1;
    std::array<char, 1024> buf2;
    std::array<iovec, 2> iov = {
        iovec{.iov_base = buf1.data(), .iov_len = buf1.size()},
        iovec{.iov_base = buf2.data(), .iov_len = buf2.size()}
    };

    auto fut = _io->readv(fd, 0, iov);

    _reactor->run();

    EXPECT_EQ(fut.get(), 2048);
    close(fd);
}

TEST_F(ReactorIOTest, IoEngineWritev) {
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf1;
    std::array<char, 1024> buf2;
    std::array<iovec, 2> iov = {
        iovec{.iov_base = buf1.data(), .iov_len = buf1.size()},
        iovec{.iov_base = buf2.data(), .iov_len = buf2.size()}
    };

    auto fut = _io->writev(fd, 0, iov);

    _reactor->run();

    EXPECT_EQ(fut.get(), 2048);
    close(fd);
}

TEST_F(ReactorIOTest, IoEngineMultipleReadv) {
    int fd = open("/dev/zero", O_RDONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf1;
    std::array<char, 1024> buf2;
    std::array<iovec, 2> iov = {
        iovec{.iov_base = buf1.data(), .iov_len = buf1.size()},
        iovec{.iov_base = buf2.data(), .iov_len = buf2.size()}
    };

    auto fut1 = _io->readv(fd, 0, iov);
    auto fut2 = _io->readv(fd, 0, iov);
    auto fut3 = _io->readv(fd, 0, iov);

    _reactor->run();

    EXPECT_EQ(fut1.get(), 2048);
    EXPECT_EQ(fut2.get(), 2048);
    EXPECT_EQ(fut3.get(), 2048);
    close(fd);
}

TEST_F(ReactorIOTest, IoEngineMultipleWritev) {
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_NE(fd, -1);

    std::array<char, 1024> buf1;
    std::array<char, 1024> buf2;
    std::array<iovec, 2> iov = {
        iovec{.iov_base = buf1.data(), .iov_len = buf1.size()},
        iovec{.iov_base = buf2.data(), .iov_len = buf2.size()}
    };

    auto fut1 = _io->writev(fd, 0, iov);
    auto fut2 = _io->writev(fd, 0, iov);
    auto fut3 = _io->writev(fd, 0, iov);

    _reactor->run();

    EXPECT_EQ(fut1.get(), 2048);
    EXPECT_EQ(fut2.get(), 2048);
    EXPECT_EQ(fut3.get(), 2048);
    close(fd);
}

TEST_F(ReactorIOTest, IoEngineOpen) {
    auto fut = _io->open("/dev/zero", O_RDONLY);

    _reactor->run();

    auto fd = fut.get();
    EXPECT_GT(fd, -1);

    close(fd);
}

TEST_F(ReactorIOTest, IoEngineOpenNonExistent) {
    auto fut = _io->open("/nonexistent", O_RDONLY);

    _reactor->run();

    auto fd = fut.get();
    EXPECT_EQ(fd, -ENOENT);
}

TEST_F(ReactorIOTest, IoEngineOpenWrite) {
    auto fut = _io->open("/dev/null", O_WRONLY);

    _reactor->run();

    auto fd = fut.get();
    EXPECT_GT(fd, -1);

    close(fd);
}

TEST_F(ReactorIOTest, IoEngineOpenReadOnly) {
    auto fut = _io->open("/dev/zero", O_RDONLY);

    _reactor->run();

    auto fd = fut.get();
    EXPECT_GT(fd, -1);

    close(fd);
}

TEST(Reactor, ReactorTwiceInit) {
    corey::Reactor reactor;
    EXPECT_DEATH({ corey::Reactor reactor2; }, ".*");
}

TEST(Reactor, IoEngineTwiceInit) {
    corey::Reactor reactor;
    corey::IoEngine io(reactor);
    EXPECT_DEATH({ corey::IoEngine io2(reactor); }, ".*");
}
