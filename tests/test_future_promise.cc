#include "reactor/future.hh"

#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>

TEST(PromiseFuture, PromiseFutureSet) {
    corey::Promise<int> test;

    auto fut = test.get_future();

    EXPECT_THROW({
      auto fut2 = test.get_future(); 
    }, corey::FutureAlreadyRetrived);

    EXPECT_FALSE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    test.set(100);

    EXPECT_TRUE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    EXPECT_EQ(fut.get(), 100);
}

TEST(PromiseFuture, PromiseSetFuture) {
    corey::Promise<int> test;

    test.set(150);

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    EXPECT_THROW({
        test.set(100);
    }, corey::PromiseAlreadySatisfied);

    EXPECT_EQ(fut.get(), 150);
}

TEST(PromiseFuture, BrokenPromiseFuture) {
    std::optional<corey::Future<int>> fut;

    {
        corey::Promise<int> test;
        fut = test.get_future();
    }

    EXPECT_TRUE(fut->is_ready());
    EXPECT_TRUE(fut->has_failed());

    EXPECT_THROW({
        (void)fut->get();
    }, corey::BrokenPromise);
}

TEST(PromiseFuture, PromiseSetExceptionFuture) {
    corey::Promise<int> test;

    test.set_exception(std::make_exception_ptr(std::runtime_error("test")));

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_TRUE(fut.has_failed());

    EXPECT_THROW(
        {(void)fut.get();},
        std::runtime_error
    );
}

TEST(PromiseFuture, PromiseVoidFutureSet) {
    corey::Promise<> test;

    auto fut = test.get_future();

    EXPECT_THROW({
      auto fut2 = test.get_future(); 
    }, corey::FutureAlreadyRetrived);

    EXPECT_FALSE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    test.set();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

}

TEST(PromiseFuture, PromiseVoidSetFuture) {
    corey::Promise<> test;
    test.set();

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    EXPECT_THROW({
      auto fut2 = test.get_future(); 
    }, corey::FutureAlreadyRetrived);

}

TEST(PromiseFuture, PromiseSetVoidExceptionFuture) {
    corey::Promise<> test;

    test.set_exception(std::make_exception_ptr(std::runtime_error("test")));

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_TRUE(fut.has_failed());

    EXPECT_THROW({ fut.get(); }, std::runtime_error);
}

TEST(PromiseFuture, PromiseSetNegativeValue) {
    corey::Promise<int> test;

    test.set(-50);

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    EXPECT_EQ(fut.get(), -50);
}

TEST(PromiseFuture, PromiseSetZeroValue) {
    corey::Promise<int> test;

    test.set(0);

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    EXPECT_EQ(fut.get(), 0);
}

TEST(PromiseFuture, PromiseSetEmptyException) {
    corey::Promise<int> test;

    EXPECT_THROW({test.set_exception(std::exception_ptr());}, std::invalid_argument);
}

TEST(PromiseFuture, PromiseVoidSetMultipleTimes) {
    corey::Promise<> test;

    test.set();
    EXPECT_THROW({test.set();}, corey::PromiseAlreadySatisfied);
}

TEST(PromiseFuture, PromiseSetExceptionMultipleTimes) {
    corey::Promise<int> test;

    test.set_exception(std::make_exception_ptr(std::runtime_error("test")));
    EXPECT_THROW(
        {test.set_exception(std::make_exception_ptr(std::runtime_error("test")));},
        corey::PromiseAlreadySatisfied
    );
}

TEST(PromiseFuture, PromiseSetExceptionMultipleTimesVoid) {
    corey::Promise<> test;

    test.set_exception(std::make_exception_ptr(std::runtime_error("test")));
    EXPECT_THROW(
        {test.set_exception(std::make_exception_ptr(std::runtime_error("test")));},
        corey::PromiseAlreadySatisfied
    );
}

class MyClass {
public:
    MyClass(int value) : value_(value) {}

    int getValue() const {
        return value_;
    }

private:
    int value_;
};

TEST(PromiseFuture, PromiseSetNonBasicClass) {
    corey::Promise<MyClass> test;

    MyClass obj(42);
    test.set(obj);

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    MyClass result = fut.get();
    EXPECT_EQ(result.getValue(), 42);
}

TEST(PromiseFuture, PromiseSetNonCopyableClass) {
    corey::Promise<std::unique_ptr<MyClass>> test;

    std::unique_ptr<MyClass> obj = std::make_unique<MyClass>(42);
    test.set(std::move(obj));

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_FALSE(fut.has_failed());

    std::unique_ptr<MyClass> result = fut.get();
    EXPECT_EQ(result->getValue(), 42);
}

TEST(PromiseFuture, PromiseSetNonCopyableClassException) {
    corey::Promise<std::unique_ptr<MyClass>> test;

    std::unique_ptr<MyClass> obj = std::make_unique<MyClass>(42);
    test.set_exception(std::make_exception_ptr(std::runtime_error("test")));

    auto fut = test.get_future();

    EXPECT_TRUE(fut.is_ready());
    EXPECT_TRUE(fut.has_failed());

    EXPECT_THROW({ std::ignore = fut.get(); }, std::runtime_error);
}
