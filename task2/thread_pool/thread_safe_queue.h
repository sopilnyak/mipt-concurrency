#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>

// Blocking queue with unlimited capacity
template<typename T>
class thread_safe_queue {
public:
    thread_safe_queue();
    thread_safe_queue(const thread_safe_queue& queue) = delete;
    void enqueue(T item);
    void pop(T& item);

private:
    std::condition_variable empty_;
    std::mutex mutex_;
    std::queue<T> queue_;
};

template<typename T>
thread_safe_queue<T>::thread_safe_queue() {
}

template<typename T>
void thread_safe_queue<T>::enqueue(T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(item);
    lock.unlock();
    empty_.notify_one();
}

template<typename T>
void thread_safe_queue<T>::pop(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto not_empty = [this] { return !queue_.empty(); };
    empty_.wait(lock, not_empty);

    item = queue_.front();
    queue_.pop();
    lock.unlock();
}