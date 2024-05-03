#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "corey.hh"

TEST(Semaphore, Wait) {
    corey::Semaphore semaphore(1);
    EXPECT_EQ(semaphore.count(), 1);
    {
        auto future = semaphore.wait();
        ASSERT_TRUE(future.is_ready());
        EXPECT_EQ(semaphore.count(), 0);
    }
    EXPECT_EQ(semaphore.count(), 1);
}

TEST(Semaphore, WaitTwice) {
    corey::Semaphore semaphore(1);
    EXPECT_EQ(semaphore.count(), 1);
    auto future1 = semaphore.wait();
    EXPECT_EQ(semaphore.count(), 0);

    auto future2 = semaphore.wait();
    EXPECT_EQ(semaphore.count(), 0);

    ASSERT_TRUE(future1.is_ready());
    ASSERT_FALSE(future2.is_ready());
}

TEST(Semaphore, WaitTwiceSignal) {
    corey::Semaphore semaphore(1);
    auto future1 = semaphore.wait();
    auto future2 = semaphore.wait();
    ASSERT_TRUE(future1.is_ready());
    ASSERT_FALSE(future2.is_ready());

    std::ignore = future1.get();
    EXPECT_EQ(semaphore.count(), 0);
    ASSERT_TRUE(future2.is_ready());
}

TEST(Semaphore, WaitTwiceSignalTwice) {
    corey::Semaphore semaphore(2);
    auto future1 = semaphore.wait();
    auto future2 = semaphore.wait();
    ASSERT_TRUE(future1.is_ready());
    ASSERT_TRUE(future2.is_ready());

    std::ignore = future1.get();
    std::ignore = future2.get();
    EXPECT_EQ(semaphore.count(), 2);
}
