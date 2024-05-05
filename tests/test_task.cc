#include "reactor/task.hh"
#include "utils/log.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <typeinfo>

TEST(TaskTest, Execute) {
    bool executed = false;
    auto task = corey::make_task([&executed]() { executed = true; });
    task.try_execute();
    EXPECT_TRUE(executed);
}

TEST(TaskTest, MoveConstructor) {
    bool executed = false;
    auto task1 = corey::make_task([&executed]() { executed = true; });
    auto task2 = std::move(task1);
    task2.try_execute();
    EXPECT_TRUE(executed);
}

TEST(TaskTest, MoveAssignmentOperator) {
    bool executed = false;
    auto task1 = corey::make_task([&executed]() { executed = true; });
    corey::Executable task2;
    task2 = std::move(task1);
    task2.try_execute();
    EXPECT_TRUE(executed);
}

class MockExecutable : public corey::AbstractExecutable {
public:
    MOCK_METHOD(bool, execute, (), (override));
};

TEST(RoutineTest, Execute) {
    auto log = corey::Log("routine-execute");

    auto routine = corey::Executable::make<MockExecutable>();    
    auto& mock = dynamic_cast<MockExecutable&>(routine.get_impl());

    EXPECT_CALL(mock, execute()).Times(1);
    routine.try_execute();
}

TEST(RoutineTest, MoveConstructor) {
    auto log = corey::Log("routine-move-constructor");

    auto routine1 = corey::Executable::make<MockExecutable>();
    auto& mock1 = dynamic_cast<MockExecutable&>(routine1.get_impl());

    auto routine2 = std::move(routine1);
    auto& mock2 = dynamic_cast<MockExecutable&>(routine2.get_impl());

    EXPECT_CALL(mock1, execute()).Times(0);
    EXPECT_CALL(mock2, execute()).Times(1);
    routine2.try_execute();
}

