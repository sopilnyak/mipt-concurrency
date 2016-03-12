#include <iostream>
#include <condition_variable>
#include <mutex>
#include <deque>

template<typename T>
class thread_safe_queue {
public:
    thread_safe_queue(std::size_t capacity);
    thread_safe_queue(const thread_safe_queue& queue) = delete;
    void enqueue(T item);
    void pop(T &item);

private:
    std::condition_variable condition_;
    std::mutex mutex_;
    std::deque<T> queue_;
    std::size_t capacity_;
};
