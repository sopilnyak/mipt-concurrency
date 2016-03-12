#include "thread_safe_queue.h"

template<typename T>
thread_safe_queue<T>::thread_safe_queue(std::size_t capacity)
        : capacity_(capacity) {
}

template<typename T>
void thread_safe_queue<T>::enqueue(T item) {
    if (queue_.size() < capacity_) {
        queue_.push_front(item);
        condition_.notify_one();
    }
}

template<typename T>
void thread_safe_queue<T>::pop(T &item) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto is_free = [this] { return !queue_.empty(); };
    condition_.wait(lock, is_free);
    queue_.pop_back();
}
