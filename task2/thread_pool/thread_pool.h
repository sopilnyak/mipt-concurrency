#include "thread_safe_queue.h"
#include "task.h"
#include <iostream>
#include <future>
#include <memory>

const int NUM_THREADS = 8;

template <typename R>
class thread_pool {
public:
    thread_pool();
    explicit thread_pool(std::size_t num_threads); // number of workers
    thread_pool(const thread_pool& lhs) = delete; // rule of 5
    thread_pool(thread_pool&& rhs) = default;

    ~thread_pool();

    thread_pool& operator=(thread_pool& lhs) = delete;
    thread_pool& operator=(thread_pool&& rhs) = default;

    std::future<R> submit(std::function<R()> function);
    void shutdown();

private:
    thread_safe_queue<task<R>> queue_;
    std::vector<std::thread> workers_;
    bool is_shutdowned_;
};

template <typename R>
thread_pool<R>::thread_pool() : is_shutdowned_(false) {
    thread_pool(NUM_THREADS);
}

template <typename R>
thread_pool<R>::thread_pool(std::size_t num_threads)
        : is_shutdowned_(false) {
    auto run = [this] {
        task<R> current;
        queue_.pop(current);
        while (!current.is_poison_pill()) {
            current.run();
            queue_.pop(current);
        }
    };
    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.push_back(std::thread(run));
    }
}

template <typename R>
thread_pool<R>::~thread_pool() {
    shutdown();
}

template <typename R>
std::future<R> thread_pool<R>::submit(std::function<R()> function) {
    if (is_shutdowned_) {
        throw std::exception();
    }
    auto promise = std::make_shared<std::promise<R>>();
    // std::shared_ptr<std::promise<R>> (promise);
    task<R> submitted(promise, function); // init new task
    queue_.enqueue(submitted);
    return promise->get_future();
}

template <typename R>
void thread_pool<R>::shutdown() {
    for (std::size_t i = 0; i < workers_.size(); ++i) {
        task<R> poison_pill;
        poison_pill.make_poison_pill();
        queue_.enqueue(poison_pill);
    }
    // shutdown all the threads correctly
    for (auto&& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    is_shutdowned_ = true;
}
