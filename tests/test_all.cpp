#include "scheduler.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(test_all, simple_add) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([]() { return 1; });
    auto task2 = scheduler.add([]() { return 2; });
    auto task3 = scheduler.add([](int a, int b) { return a + b; },
        scheduler.getFutureResult<int>(task1), scheduler.getFutureResult<int>(task2));
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<int>(task3), 3);
}

TEST(test_all, add_with_args) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([](int a, int b) { return a + b; }, 1, 2);
    auto task2 = scheduler.add([](int a, int b) { return a + b; }, 3, 4);
    auto task3 = scheduler.add([](int a, int b) { return a + b; },
        scheduler.getFutureResult<int>(task1), scheduler.getFutureResult<int>(task2));
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<int>(task3), 10);
}

TEST(test_all, StringOperations) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([]() { return std::string("Hello"); });
    auto task2 = scheduler.add([]() { return std::string("World"); });
    auto task3 = scheduler.add([](std::string a, std::string b) { return a + " " + b; },
        scheduler.getFutureResult<std::string>(task1),
        scheduler.getFutureResult<std::string>(task2));
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<std::string>(task3), "Hello World");
}

TEST(test_all, FloatOperations) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([]() { return 1.5f; });
    auto task2 = scheduler.add([]() { return 2.5f; });
    auto task3 = scheduler.add([](float a, float b) { return a + b; },
        scheduler.getFutureResult<float>(task1),
        scheduler.getFutureResult<float>(task2));
    scheduler.executeAll();
    ASSERT_FLOAT_EQ(scheduler.getResult<float>(task3), 4.0f);
}

TEST(test_all, ComplexOperations) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([]() { return 3.0; });
    auto task2 = scheduler.add([]() { return 4.0; });
    auto task3 = scheduler.add([](double a, double b) { return a * b; },
        scheduler.getFutureResult<double>(task1),
        scheduler.getFutureResult<double>(task2));
    auto task4 = scheduler.add([](double a) { return a + 1.0; },
        scheduler.getFutureResult<double>(task3));
    scheduler.executeAll();
    ASSERT_DOUBLE_EQ(scheduler.getResult<double>(task4), 13.0);
}

TEST(test_all, LambdaCapture) {
    TTaskScheduler scheduler;
    int x = 5;
    auto task1 = scheduler.add([x]() { return x * 2; });
    auto task2 = scheduler.add([](int a) { return a + 3; },
        scheduler.getFutureResult<int>(task1));
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<int>(task2), 13);
}

TEST(test_all, MemberFunction) {
    TTaskScheduler scheduler;
    struct MyClass {
        int value;
        int add(int a) { return value + a; }
    };
    MyClass obj{ 10 };
    auto task1 = scheduler.add(&MyClass::add, obj, 5);
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<int>(task1), 15);
}

TEST(test_all, MemberFunctionWithCapture) {
    TTaskScheduler scheduler;
    struct MyClass {
        int value;
        int add(int a) { return value + a; }
    };
    MyClass obj{ 10 };
    auto task1 = scheduler.add([&obj](int a) { return obj.add(a); }, 5);
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<int>(task1), 15);
}

TEST(test_all, TaskWithNoDependencies) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([]() { return 42; });
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<int>(task1), 42);
}

TEST(test_all, ExceptionHandling) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([]() -> int { throw std::runtime_error("Error"); return 0; });
    ASSERT_THROW(scheduler.executeAll(), std::runtime_error);
}

TEST(test_all, CircularDependency) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([](int a) { return a; }, scheduler.getFutureResult<int>(1));
    auto task2 = scheduler.add([](int a) { return a; }, scheduler.getFutureResult<int>(0));
    ASSERT_THROW(scheduler.executeAll(), std::runtime_error);
}

TEST(test_all, LargeNumberOfTasks) {
    TTaskScheduler scheduler;
    const int num_tasks = 1000;
    std::vector<int> task_ids;
    for (int i = 0; i < num_tasks; ++i) {
        task_ids.push_back(scheduler.add([i]() { return i; }));
    }
    scheduler.executeAll();
    for (int i = 0; i < num_tasks; ++i) {
        ASSERT_EQ(scheduler.getResult<int>(task_ids[i]), i);
    }
}

TEST(test_all, MultipleDependencies) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([]() { return 1; });
    auto task2 = scheduler.add([]() { return 2; });
    auto task3 = scheduler.add([](int a, int b) { return a * b; },
        scheduler.getFutureResult<int>(task1), scheduler.getFutureResult<int>(task2));
    auto task4 = scheduler.add([](int a, int b) { return a + b; },
        scheduler.getFutureResult<int>(task3), scheduler.getFutureResult<int>(task2));
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<int>(task4), 4);
}

TEST(test_all, VoidTask) {
    TTaskScheduler scheduler;
    bool executed = false;
    auto task1 = scheduler.add([&executed]() { executed = true; });
    scheduler.executeAll();
    ASSERT_TRUE(executed);
}

TEST(test_all, MixedDataTypes) {
    TTaskScheduler scheduler;
    auto task1 = scheduler.add([]() { return 42; });
    auto task2 = scheduler.add([]() { return std::string("Hello"); });
    auto task3 = scheduler.add([](int a, std::string b) { return b + " " + std::to_string(a); },
        scheduler.getFutureResult<int>(task1), scheduler.getFutureResult<std::string>(task2));
    scheduler.executeAll();
    ASSERT_EQ(scheduler.getResult<std::string>(task3), "Hello 42");
}
