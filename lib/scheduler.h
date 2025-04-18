#pragma once

#include <memory>
#include <vector>
#include <queue>

class Base {
public:
    virtual ~Base() = default;
};
template<typename T>
class Derived : public Base {
public:
    T value;
    explicit Derived(T&& value) : value(std::forward<T>(value)) {}
};
class MyAny {
public:
    std::unique_ptr<Base> ptr;
    template<typename T>
    MyAny(T&& value) : ptr(std::make_unique<Derived<T>>(std::forward<T>(value))) {}
    template<typename T>
    T get() const {
        auto* derived = dynamic_cast<Derived<T>*>(ptr.get());
        return derived->value;
    }
};

class TTaskScheduler;

template<typename TResult>
class FutureResult {
public:
    TTaskScheduler* scheduler;
    int task_id;

    FutureResult(TTaskScheduler* sched, int id) : scheduler(sched), task_id(id) {}
    int getTaskId() const {
        return task_id;
    }
    TResult get();
};

template<typename T>
class is_future_result {
public:
    static constexpr bool value = false;
};
template<typename TResult>
class is_future_result<FutureResult<TResult>> {
public:
    static constexpr bool value = true;
};
template<typename T>
constexpr bool is_future_result_v = is_future_result<std::decay_t<T>>::value;

class TaskBase {
public:
    MyAny result = 0;
    bool completed = false;
    std::vector<int> dependencies;

    virtual ~TaskBase() = default;
    virtual void execute() = 0;
    bool isCompleted() const {
        return completed;
    }
    const std::vector<int>& getDependencies() const {
        return dependencies;
    }
    template<typename T>
    void collect_dependency(std::vector<int>& dependencies, const T& arg) {
        if constexpr (is_future_result_v<T>) {
            dependencies.push_back(arg.getTaskId());
        }
    }
    template<typename T>
    auto resolve_argument(T&& arg) {
        if constexpr (is_future_result_v<std::decay_t<T>>) {
            return arg.get();
        } else {
            return std::forward<T>(arg);
        }
    }
};

template<typename TFunc>
class Task_zero_args : public TaskBase {
public:
    TFunc func;

    explicit Task_zero_args(TFunc&& f) : func(std::forward<TFunc>(f)) {}
    void execute() override {
        if constexpr (std::is_void_v<decltype(func())>) {
            func();
        } else {
            result = MyAny(func());
        }
        completed = true;
    }
};

template<typename TFunc, typename TArg1>
class Task_one_args : public TaskBase {
public:
    TFunc func;
    TArg1 arg1;

    Task_one_args(TFunc&& f, TArg1&& a1)
        : func(std::forward<TFunc>(f)), arg1(std::forward<TArg1>(a1)) {
        collect_dependency(dependencies,arg1);
    }
    void execute() override {
        result = func(resolve_argument(arg1));
        completed = true;
    }
};

template<typename TFunc, typename TArg1, typename TArg2>
class Task_two_args : public TaskBase {
public:
    TFunc func;
    TArg1 arg1;
    TArg2 arg2;

    Task_two_args(TFunc&& f, TArg1&& a1, TArg2&& a2)
        : func(std::forward<TFunc>(f)), arg1(std::forward<TArg1>(a1)), arg2(std::forward<TArg2>(a2)) {
        collect_dependency(dependencies, arg1);
        collect_dependency(dependencies, arg2);
    }
    void execute() override {
        if constexpr (std::is_member_function_pointer_v<std::decay_t<TFunc>>) {
            auto obj = resolve_argument(arg1);
            auto param = resolve_argument(arg2);
            result = (obj.*func)(param);
        } else {
            result = func(resolve_argument(arg1), resolve_argument(arg2));
        }
        completed = true;
    }
};

class TTaskScheduler {
public:
    std::vector<std::unique_ptr<TaskBase>> tasks;

    template<typename TFunc>
    int add(TFunc&& func) {
        auto task = std::make_unique<Task_zero_args<TFunc>>(std::forward<TFunc>(func));
        tasks.emplace_back(std::move(task));
        return tasks.size() - 1;
    }
    template<typename TFunc, typename TArg1>
    int add(TFunc&& func, TArg1&& arg1) {
        auto task = std::make_unique<Task_one_args<TFunc, TArg1>>(std::forward<TFunc>(func),
            std::forward<TArg1>(arg1));
        tasks.emplace_back(std::move(task));
        return tasks.size() - 1;
    }
    template<typename TFunc, typename TArg1, typename TArg2>
    int add(TFunc&& func, TArg1&& arg1, TArg2&& arg2) {
        auto task = std::make_unique<Task_two_args<TFunc, TArg1, TArg2>>(std::forward<TFunc>(func),
            std::forward<TArg1>(arg1), std::forward<TArg2>(arg2));
        tasks.emplace_back(std::move(task));
        return tasks.size() - 1;
    }
    template<typename TResult>
    TResult getResult(int task_id) {
        auto& task = tasks[task_id];
        if (!task->isCompleted()) {
            task->execute();
        }
        return task->result.get<TResult>();
    }
    template<typename TResult>
    FutureResult<TResult> getFutureResult(int task_id) {
        return FutureResult<TResult>(this, task_id);
    }
    void executeAll() {
        std::vector<int> order;
        std::vector<std::vector<int>> adj(tasks.size());
        std::vector<int> in_degree(tasks.size(), 0);

        for (int i = 0; i < tasks.size(); ++i) {
            for (int dep : tasks[i]->getDependencies()) {
                adj[dep].push_back(i);
                in_degree[i]++;
            }
        }
        std::queue<int> q;
        for (int i = 0; i < tasks.size(); ++i) {
            if (in_degree[i] == 0) q.push(i);
        }
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            order.push_back(u);
            for (int v : adj[u]) {
                if (--in_degree[v] == 0) q.push(v);
            }
        }
        if (order.size() != tasks.size()) {
            throw std::runtime_error("Обнаружен цикл в зависимостях ");
        }
        for (int id : order) {
            if (!tasks[id]->isCompleted()) {
                tasks[id]->execute();
            }
        }
    }
};

template<typename TResult>
TResult FutureResult<TResult>::get() {
    return scheduler->getResult<TResult>(task_id);
}
