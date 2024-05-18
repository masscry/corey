#include "reactor/task.hh"
#include "utils/log.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <typeinfo>

TEST(TaskTest, Execute) {
    bool executed = false;
    auto task = corey::make_task([&executed]() { executed = true; });
    task();
    EXPECT_TRUE(executed);
}

TEST(TaskTest, MoveConstructor) {
    bool executed = false;
    auto task1 = corey::make_task([&executed]() { executed = true; });
    auto task2 = std::move(task1);
    task2();
    EXPECT_TRUE(executed);
}

TEST(TaskTest, MoveAssignmentOperator) {
    bool executed = false;
    auto task1 = corey::make_task([&executed]() { executed = true; });
    corey::Task task2;
    task2 = std::move(task1);
    task2();
    EXPECT_TRUE(executed);
}

class MockRoutine : public corey::AbstractExecutable<void> {
public:
    MOCK_METHOD(void, execute, (), (override));
};

TEST(RoutineTest, Execute) {
    auto log = corey::Log("routine-execute");

    auto routine = corey::Routine::make<MockRoutine>();    
    auto& mock = dynamic_cast<MockRoutine&>(routine.get_impl());

    EXPECT_CALL(mock, execute()).Times(1);
    routine();
}

TEST(RoutineTest, MoveConstructor) {
    auto log = corey::Log("routine-move-constructor");

    auto routine1 = corey::Routine::make<MockRoutine>();
    auto& mock1 = dynamic_cast<MockRoutine&>(routine1.get_impl());

    auto routine2 = std::move(routine1);
    auto& mock2 = dynamic_cast<MockRoutine&>(routine2.get_impl());

    EXPECT_CALL(mock1, execute()).Times(0);
    EXPECT_CALL(mock2, execute()).Times(1);
    routine2();
}

