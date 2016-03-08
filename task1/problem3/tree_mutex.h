#include <iostream>
#include <vector>
#include <atomic>
#include <array>

class mutex {
public:
    mutex();
    void lock(int thread_index);
    void unlock(int thread_index);

private:
    std::array<std::atomic<bool>, 2> want_;
    std::atomic<int> victim_;
};

class tree_mutex {
public:
    tree_mutex(std::size_t num_threads);
    void lock(std::size_t thread_index);
    void unlock(std::size_t thread_index);

private:
    std::size_t tree_size_(std::size_t num_threads); // get size of the tree
    std::vector<mutex> mutexes_; // tree of mutexes
    std::size_t levels_; // height of the tree
};
