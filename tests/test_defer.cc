#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <corey.hh>

TEST(DeferTest, SingleDefer) {
    bool executed = false;
    {
        auto def = corey::defer([&]() noexcept {
            executed = true;
        });
    }
    EXPECT_TRUE(executed);
}

TEST(DeferTest, DeferWithCancel) {
    bool executed = false;
    {
        auto def = corey::defer([&]() noexcept {
            executed = true;
        });
        def.cancel();
    }
    EXPECT_FALSE(executed);
}

TEST(DeferTest, MultipleDefers) {
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

TEST(DeferTest, MultipleDefersWithCancel) {
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

TEST(DeferTest, MultipleDefersWithCancelAll) {
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

TEST(DeferTest, DeferWithException) {
    bool executed = false;
    try {
        auto def = corey::defer([&]() noexcept {
            executed = true;
        });
        throw std::runtime_error("Exception occurred");
    } catch (const std::exception& e) {}
    EXPECT_TRUE(executed);
}

TEST(DeferTest, DeferWithExceptionAndCancel) {
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

TEST(DeferTest, DeferWithExceptionAndCancelMultiple) {
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

TEST(DeferTest, DeferWithExceptionAndCancelAll) {
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

TEST(DeferTest, ReturnDeferFromFuncTypeErased) {
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

TEST(DeferTest, ReturnDeferFromFuncTypeErasedWithCancel) {
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

TEST(DeferTest, ReturnDeferFromFuncTypeErasedWithException) {
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

TEST(DeferTest, ReturnDeferFromFuncTypeErasedWithExceptionAndCancel) {
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

TEST(DeferTest, ReturnDeferFromFuncTypeErasedWithExceptionAndCancelMultiple) {
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

