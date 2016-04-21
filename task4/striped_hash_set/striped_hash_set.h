#include <iostream>
#include <vector>
#include <forward_list>
#include <shared_mutex>

const int DEFAULT_GROWTH_FACTOR = 2;
const float DEFAULT_LOAD_FACTOR = 1.5;

template <typename T, class H = std::hash<T>>
class striped_hash_set {
public:
    explicit striped_hash_set(size_t num_stripes,
                              int growth_factor = DEFAULT_GROWTH_FACTOR,
                              float load_factor = DEFAULT_LOAD_FACTOR);
    striped_hash_set(striped_hash_set&) = delete;
    striped_hash_set(striped_hash_set&&) = default;
    striped_hash_set& operator=(striped_hash_set&) = delete;
    striped_hash_set& operator=(striped_hash_set&&) = default;
    ~striped_hash_set() = default;

    void add(const T& element);
    void remove(const T& element);
    bool contains(const T& element);

private:
    std::vector<std::forward_list<T>> hash_table_;
    std::vector<std::shared_timed_mutex> mutex_array_;

    float load_factor_;
    int growth_factor_;
    int num_elements_;

    H hash_function_;
    void rehash();
};

template <typename T, class H>
striped_hash_set<T, H>::striped_hash_set(size_t num_stripes,
                                   int growth_factor,
                                   float load_factor)
        : growth_factor_(growth_factor),
          load_factor_(load_factor),
          num_elements_(0),
          mutex_array_(num_stripes),
          hash_table_(num_stripes) {
}

template <typename T, class H>
void striped_hash_set<T, H>::add(const T& element) {
    int hash = hash_function_(element);
    if (num_elements_ / hash_table_.size() < load_factor_) {
        std::unique_lock<std::shared_timed_mutex> lock(
                mutex_array_[hash % mutex_array_.size()]);
        for (auto current : hash_table_[hash % hash_table_.size()]) {
            if (current == element) {
                return;
            }
        }
        hash_table_[hash % hash_table_.size()].push_front(element);
        ++num_elements_;
        return;
    }

    std::vector<std::unique_lock<std::shared_timed_mutex>> lock_array;
    // lock the first mutex
    lock_array.emplace_back(mutex_array_[0]);

    // check load factor once again to prevent from data race
    std::size_t hash_table_size = hash_table_.size();
    if (hash_table_.size() != hash_table_size) {
        return;
    }

    // lock the rest of mutexes
    for (std::size_t i = 1; i < mutex_array_.size(); ++i) {
        lock_array.emplace_back(mutex_array_[i]);
    }
    rehash();
}

template <typename T, class H>
void striped_hash_set<T, H>::rehash() {
    std::vector<std::forward_list<T>> new_hash_table;
    new_hash_table.resize(hash_table_.size() * growth_factor_);
    for (std::forward_list<T> bucket : hash_table_) {
        for (auto current : bucket) {
            int hash = hash_function_(current) % new_hash_table.size();
            new_hash_table[hash].push_front(current);
        }
    }
    hash_table_ = std::move(new_hash_table);
}

template <typename T, class H>
void striped_hash_set<T, H>::remove(const T& element) {
    int hash = hash_function_(element);
    std::unique_lock<std::shared_timed_mutex> lock(
            mutex_array_[hash % mutex_array_.size()]);
    for (auto current : hash_table_[hash % hash_table_.size()]) {
        if (current == element) {
            hash_table_[hash % hash_table_.size()].remove(element);
        }
    }
}

template <typename T, class H>
bool striped_hash_set<T, H>::contains(const T& element) {
    int hash = hash_function_(element);
    std::unique_lock<std::shared_timed_mutex> lock(
            mutex_array_[hash % mutex_array_.size()]);
    for (auto current : hash_table_[hash % hash_table_.size()]) {
        if (current == element) {
            return true;
        }
    }
    return false;
}
