#include <atomic>
#include <memory>

template<class T>
class lock_free_queue {
public:
    lock_free_queue();
    lock_free_queue(const lock_free_queue&) = delete;
    lock_free_queue(lock_free_queue&&)= default;
    lock_free_queue& operator=(const lock_free_queue&) = delete;
    lock_free_queue& operator=(lock_free_queue&&) = default;
    ~lock_free_queue();

    void enqueue(T item);
    bool dequeue(T& item);

private:
    const static int INTERNAL_COUNT = 30;
    const static int EXTERNAL_COUNTERS = 2;

    struct pointer_t_;
    struct node_counter_;
    struct node_t_ {
        std::atomic<T> value;
        std::atomic<node_counter_> count;
        pointer_t_ next;

        node_t_() noexcept {
            next.ptr = nullptr;
            next.external_count = 0;

            node_counter_ new_count;
            new_count.internal_count = 0;
            new_count.external_counters = 2;
            count.store(new_count);
        }
        void release_ref() {
            node_counter_ old_counter = count.load(std::memory_order_relaxed);
            node_counter_ new_counter;

            do {
                new_counter = old_counter;
                --new_counter.internal_count;
            } while(!count.compare_exchange_strong(old_counter, new_counter,
                                                   std::memory_order_acquire,
                                                   std::memory_order_relaxed));
            if (!new_counter.internal_count && !new_counter.external_counters) {
                delete this;
            }
        }
    };
    struct node_counter_ {
        int internal_count : INTERNAL_COUNT;
        int external_counters : EXTERNAL_COUNTERS;
    };
    struct pointer_t_ {
        pointer_t_() noexcept : ptr(nullptr), external_count(0) {}
        pointer_t_(pointer_t_&) noexcept = default;
        pointer_t_(pointer_t_&&) noexcept = default;
        pointer_t_& operator=(const pointer_t_&) noexcept = default;
        pointer_t_& operator=(pointer_t_&&) noexcept = default;
        ~pointer_t_() noexcept = default;
        node_t_* ptr;
        int external_count;
    };

    void increase_internal_count_(
            std::atomic<pointer_t_> &counter, pointer_t_ &old_counter);
    void free_external_counter_(pointer_t_ &old_node_ptr);

    std::atomic<pointer_t_> head_;
    std::atomic<pointer_t_> tail_;
};

template<class T>
lock_free_queue<T>::lock_free_queue() {
    pointer_t_ pointer;
    pointer.ptr = new node_t_();
    pointer.external_count = 0;
    tail_.store(pointer);
    head_.store(tail_.load());
}

template<class T>
void lock_free_queue<T>::enqueue(T item) {
    std::unique_ptr<T> new_value(new T(item));
    pointer_t_ new_next;
    new_next.ptr = new node_t_;
    new_next.external_count = 1;
    pointer_t_ old_tail = tail_.load();

    while(true) {
        increase_internal_count_(tail_, old_tail);
        T* old_value = nullptr;
        if (old_tail.ptr->value.compare_exchange_strong(
                old_value, new_value->get())) {
            old_tail.ptr->next = new_next;
            old_tail = tail_.exchange(new_next);
            free_external_counter_(old_tail);
            new_value.release();
            break;
        }
        old_tail.ptr->release_ref();
    }
}

template<class T>
bool lock_free_queue<T>::dequeue(T &item) {
    pointer_t_ old_head = head_.load(std::memory_order_relaxed);
    while(true) {
        increase_internal_count_(head_, old_head);
        node_t_ * const ptr = old_head.ptr;
        if (ptr == tail_.load().ptr) {
            ptr->release_ref();
            return false;
        }
        if (head_.compare_exchange_strong(old_head, ptr->next)) {
            item = ptr->value;
            free_external_counter_(old_head);
            return true;
        }
        ptr->release_ref();
    }
}

template<class T>
void lock_free_queue<T>::increase_internal_count_(
        std::atomic<pointer_t_> &counter, pointer_t_ &old_counter) {
    pointer_t_ new_counter;
    do {
        new_counter = old_counter;
        ++new_counter.external_count;
    } while(counter.compare_exchange_strong(old_counter, new_counter,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed));
    old_counter.external_count = new_counter.external_count;
}

template<class T>
void lock_free_queue<T>::free_external_counter_(pointer_t_ &old_node_ptr) {
    node_t_ * const ptr = old_node_ptr.ptr;
    const int count_increase = old_node_ptr.external_count - 2;
    node_counter_ old_counter = ptr->count.load(std::memory_order_relaxed);
    node_counter_ new_counter;
    do {
        new_counter = old_counter;
        --new_counter.external_counters;
        new_counter.internal_count += count_increase;
    } while(!ptr->count.compare_exchange_strong(old_counter, new_counter,
                                                std::memory_order_acquire,
                                                std::memory_order_relaxed));
    if (!new_counter.internal_count && !new_counter.external_counters) {
        delete ptr;
    }
}

template<class T>
lock_free_queue<T>::~lock_free_queue() {
    T item;
    while(dequeue(item)) {}
}
