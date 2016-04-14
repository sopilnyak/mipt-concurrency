#include <iostream>
#include <condition_variable>
#include <mutex>

class barrier {
public:
    explicit barrier(size_t num_threads);
    void enter();

private:
    const size_t num_threads_;
    std::condition_variable opened_barrier_;
    std::condition_variable closed_barrier_;
    std::mutex mutex_;
    size_t threads_left_;
    bool is_barrier_open;
};

barrier::barrier(size_t num_threads)
        : num_threads_(num_threads), threads_left_(num_threads), is_barrier_open(true) {
}

void barrier::enter() {
    std::unique_lock<std::mutex> lock(mutex_);
    // wait until the barrier is opened
    closed_barrier_.wait(lock, [this] { return is_barrier_open; });

    --threads_left_;
    if (threads_left_ > 0) {
        opened_barrier_.wait(lock);
    } else {
        is_barrier_open = false;
        opened_barrier_.notify_all();
    }

    ++threads_left_;
    if (threads_left_ == num_threads_) {
        is_barrier_open = true;
        closed_barrier_.notify_all();
    }
}