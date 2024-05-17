#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <future>
#include <queue>
#include <functional>

class ThreadPool
{
public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 单例模式
    static ThreadPool& getInstance() {
        static ThreadPool ins;
        return ins;
    }

    using Task = std::packaged_task<void()>;

    template<class F, class... Args>	// 尾置类型推导：尖括号后面的是返回值类型
    auto commit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using RetType = decltype(f(args...));
        if (stop_.load()) {
            return std::future<RetType>{};
        }
        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<RetType> ret = task->get_future();
        {
            std::lock_guard<std::mutex> cv_mt(cv_mt_);
            // 由于task是一个智能指针，所以这里可以保证，task在没有执行之前一直都是安全的。
            tasks_.emplace([task] {(*task)(); });
        }
        cv_lock_.notify_one();
        return ret;
    }

    void stop();

    bool isStop();

    ~ThreadPool() {
        if (!stop_) {
            stop();
        }
    }

private:

    ThreadPool(unsigned int thread_num = std::thread::hardware_concurrency() / 2);

    void start();

    std::mutex cv_mt_;
    std::condition_variable cv_lock_;
    std::atomic_bool stop_;
    std::atomic_int thread_num_;
    std::queue<Task> tasks_;
    std::vector<std::thread> pool_;
};

#endif // THREADPOOL_H
