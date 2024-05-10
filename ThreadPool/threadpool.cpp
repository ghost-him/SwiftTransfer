#include "threadpool.h"

ThreadPool::ThreadPool(unsigned int thread_num) {
    thread_num_ = thread_num;
    stop_ = true;
    start();
}

void ThreadPool::start()
{
    stop_ = false;
    for (int i = 0; i < thread_num_; i++) {
        pool_.emplace_back([this]() {
            while (!this->stop_.load()) {
                Task task;
                {
                    std::unique_lock<std::mutex> cv_mt(cv_mt_);
                    this->cv_lock_.wait(cv_mt, [this] {
                        return this->stop_.load() || !this->tasks_.empty();
                    });
                    if (stop_)
                        return;
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                this->thread_num_--;
                task();
                this->thread_num_++;
            }
        });
    }
}

void ThreadPool::stop()
{
    stop_.store(true);
    cv_lock_.notify_all();
    for (auto& td : pool_) {
        if (td.joinable()) {
            td.join();
        }
    }
}
