#include <vector>
#include <atomic>

template<class Value>
class spsc_ring_buffer {
public:
    explicit spsc_ring_buffer(const std::size_t capacity);
    spsc_ring_buffer(spsc_ring_buffer&) = delete;
    spsc_ring_buffer(spsc_ring_buffer&&) = default;
    spsc_ring_buffer& operator=(spsc_ring_buffer&) = delete;
    spsc_ring_buffer& operator=(spsc_ring_buffer&&) = default;
    ~spsc_ring_buffer() = default;

    bool enqueue(Value element);
    bool dequeue(Value& element);

private:
    std::vector<Value> buffer_;
    std::size_t capacity_;

    std::atomic<std::size_t> head_;
    std::atomic<std::size_t> tail_;

    std::size_t next_(std::size_t current) const noexcept {
        return (current + 1) % capacity_;
    }
};

template<class Value>
spsc_ring_buffer<Value>::spsc_ring_buffer(const std::size_t capacity_)
        : buffer_(capacity_ + 1),
          capacity_(capacity_ + 1),
          head_(0),
          tail_(0) {
}

template<class Value>
bool spsc_ring_buffer<Value>::enqueue(Value element) {
    std::size_t head = head_.load(std::memory_order_relaxed);
    std::size_t next_head = next_(head);
    if (next_head == tail_.load(std::memory_order_acquire)) {
        return false;
    }
    buffer_[head] = element;
    head_.store(next_head, std::memory_order_release);
    return true;
}

template<class Value>
bool spsc_ring_buffer<Value>::dequeue(Value& element) {
    std::size_t tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) {
        return false;
    }
    element = buffer_[tail];
    tail_.store(next_(tail), std::memory_order_release);
    return true;
}
