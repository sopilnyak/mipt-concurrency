#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>

class tree_mutex {
  private:
    std::size_t num_threads;
    std::vector<std::size_t> want;
    std::vector<std::size_t> turn;
    bool exists(std::size_t tree_index, std::size_t index);

  public:
    tree_mutex(std::size_t num_threads);
    void lock(std::size_t tree_index);
    void unlock(std::size_t tree_index);
};

tree_mutex::tree_mutex(std::size_t num_threads) {
    this->num_threads = num_threads;
    want.resize(num_threads, 0);
    turn.resize(num_threads - 1, 0);
}

void tree_mutex::lock(std::size_t tree_index) {
    for (std::size_t i = 0; i < num_threads - 1; ++i) {
        want[tree_index] = i + 1;
        turn[i] = tree_index;
        while (exists(tree_index, i) && turn[i] == tree_index) {
            std::this_thread::yield(); // wait
        }
    }
}

bool tree_mutex::exists(std::size_t tree_index, std::size_t index) {
    auto check = [index](std::size_t value) { return value >= index + 1; };
    return std::any_of(want.begin(), want.begin() + tree_index, check)
           || std::any_of(want.begin() + tree_index + 1, want.end(), check);
}

void tree_mutex::unlock(std::size_t tree_index) {
    want[tree_index] = 0;
}
