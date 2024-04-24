#include "reactor/task.hh"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(TaskTest, Execute) {
    bool executed = false;
    auto func = [&executed]() { executed = true; };
    corey::Task task = corey::make_task(func);
    task.execute();
    EXPECT_TRUE(executed);
}

TEST(TaskTest, CopyConstructor) {
    bool executed = false;
    auto func = [&executed]() { executed = true; };
    corey::Task task1 = corey::make_task(func);
    corey::Task task2 = task1;
    task2.execute();
    EXPECT_TRUE(executed);
}

TEST(TaskTest, MoveConstructor) {
    bool executed = false;
    auto func = [&executed]() { executed = true; };
    corey::Task task1 = corey::make_task(func);
    corey::Task task2 = std::move(task1);
    task2.execute();
    EXPECT_TRUE(executed);
}

TEST(TaskTest, CopyAssignmentOperator) {
    bool executed = false;
    auto func = [&executed]() { executed = true; };
    corey::Task task1 = corey::make_task(func);
    corey::Task task2;
    task2 = task1;
    task2.execute();
    EXPECT_TRUE(executed);
}

TEST(TaskTest, MoveAssignmentOperator) {
    bool executed = false;
    auto func = [&executed]() { executed = true; };
    corey::Task task1 = corey::make_task(func);
    corey::Task task2;
    task2 = std::move(task1);
    task2.execute();
    EXPECT_TRUE(executed);
}

