#include <iostream>
#include <future>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <queue>

const int NUM_THREADS = 8;

// Blocking queue with unlimited capacity
template <typename T>
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

template <typename T>
thread_safe_queue<T>::thread_safe_queue() {
}

template <typename T>
void thread_safe_queue<T>::enqueue(T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push(item);
    lock.unlock();
    empty_.notify_one();
}

template <typename T>
void thread_safe_queue<T>::pop(T& item) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto not_empty = [this] { return !queue_.empty(); };
    empty_.wait(lock, not_empty);

    item = queue_.front();
    queue_.pop();
    lock.unlock();
}

template <class R>
class task {
public:
    task();
    task(std::shared_ptr<std::promise<R>> promise, std::function<R()> function);
    void run() const;
    bool is_poison_pill() const;
    void make_poison_pill();

private:
    bool is_poison_pill_;
    std::function<R()> function_;
    std::shared_ptr<std::promise<R>> promise_;
};

template <class R>
task<R>::task()
        : is_poison_pill_(false) {
}

template <class R>
task<R>::task(std::shared_ptr<std::promise<R>> promise, std::function<R()> function)
        : is_poison_pill_(false), function_(function), promise_(promise) {
}

template <class R>
void task<R>::run() const {
    R returned;
    try {
        returned = function_();
    } catch (...) {
        promise_->set_exception(std::current_exception());
        return;
    }
    promise_->set_value(returned);
}

template <class R>
bool task<R>::is_poison_pill() const {
    return is_poison_pill_;
}

template <class R>
void task<R>::make_poison_pill() {
    is_poison_pill_ = true;
}

template <typename R>
class thread_pool {
public:
    thread_pool();
    explicit thread_pool(std::size_t num_threads); // number of workers
    ~thread_pool();
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