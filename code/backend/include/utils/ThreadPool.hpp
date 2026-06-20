#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std;

namespace alphaforge {

// A classic fixed-size thread pool. submit() returns a future so callers
// can fan work out across symbols (parallel risk, parallel analytics) and then
// join the results. Correct on any core count; on a single core it simply
// serialises without a wall clock speedup.
class ThreadPool {
public:
    explicit ThreadPool(size_t threads =
                            max<size_t>(1, thread::hardware_concurrency())) {
        workers_.reserve(threads);
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&)                 = delete;
    ThreadPool& operator=(ThreadPool&&)      = delete;

    ~ThreadPool() {
        {
            scoped_lock lock(mutex_);
            stopping_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) {
                w.join();
            }
        }
    }

    template <typename F, typename... Args>
    [[nodiscard]] auto submit(F&& f, Args&&... args)
        -> future<invoke_result_t<F, Args...>> {
        using Return = invoke_result_t<F, Args...>;

        auto task = make_shared<packaged_task<Return()>>(
            [fn = forward<F>(f),
             ... a = forward<Args>(args)]() mutable -> Return {
                return fn(a...);
            });

        future<Return> fut = task->get_future();
        {
            scoped_lock lock(mutex_);
            if (stopping_) {
                throw runtime_error("submit on a stopped ThreadPool");
            }
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_one();
        return fut;
    }

    [[nodiscard]] size_t size() const noexcept { return workers_.size(); }

private:
    void worker_loop() {
        for (;;) {
            function<void()> job;
            {
                unique_lock lock(mutex_);
                cv_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
                if (stopping_ && tasks_.empty()) {
                    return;
                }
                job = move(tasks_.front());
                tasks_.pop();
            }
            job();
        }
    }

    vector<thread>          workers_;
    queue<function<void()>> tasks_;
    mutex                        mutex_;
    condition_variable           cv_;
    bool                              stopping_{false};
};

} // namespace alphaforge
