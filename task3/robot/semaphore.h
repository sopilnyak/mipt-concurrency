#include <mutex>
#include <atomic>
#include <condition_variable>

class semaphore {
public:
    semaphore();
    void wait();
    void signal();

private:
    std::mutex mutex_;
    std::atomic<int> count_;
    std::condition_variable signal_cv_;
};

semaphore::semaphore() : count_(0) { }

void semaphore::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    while(count_.load() == 0) {
        signal_cv_.wait(lock);
    }
    count_.store(count_.load() - 1);
}

void semaphore::signal() {
    std::lock_guard<std::mutex> lock(mutex_);
    count_.store(count_.load() + 1);
    signal_cv_.notify_one();
}
