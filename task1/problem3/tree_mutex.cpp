#include "tree_mutex.h"
#include <thread>
#include <algorithm>

mutex::mutex() {
    want_[0].store(false);
    want_[1].store(false);
    victim_.store(0);
}

void mutex::lock(int thread_index) {
    want_[thread_index].store(true);
    victim_.store(thread_index);
    while (want_[1 - thread_index].load() && victim_.load() == thread_index) {
        std::this_thread::yield(); // wait
    }
}

void mutex::unlock(int thread_index) {
    want_[thread_index].store(false);
}

tree_mutex::tree_mutex(std::size_t num_threads) : mutexes_(tree_size_(num_threads)) {
    levels_ = static_cast<std::size_t>(ceil(log2(num_threads)));
}

void tree_mutex::lock(std::size_t thread_index) {
    auto vertex = static_cast<int>(mutexes_.size() - pow(2, levels_) + thread_index);
    // traverse the tree from leaves to root
    while (vertex > 0) {
        if (vertex % 2 == 0) {
            vertex = (vertex - 2) / 2;
            mutexes_[vertex].lock(0);
        } else {
            vertex = (vertex - 1) / 2;
            mutexes_[vertex].lock(1);
        }
    }
}

void tree_mutex::unlock(std::size_t thread_index) {
    std::vector<int> path;
    auto vertex = static_cast<int>(mutexes_.size() - pow(2, levels_) + thread_index);
    while (vertex >= 0) { // build path from leaves to root
        path.push_back(vertex);
        if (vertex % 2 == 0) {
            vertex = (vertex - 2) / 2;
        } else {
            vertex = (vertex - 1) / 2;
        }
    }
    // traverse the tree from root to leaves
    for (auto i = path.rbegin(); i < path.rend() - 1; ++i) {
        mutexes_[*i].unlock(*(i + 1) % 2); // unlock mutexes
    }
}

std::size_t tree_mutex::tree_size_(std::size_t num_threads) {
    auto levels = static_cast<std::size_t>(ceil(log2(num_threads)));
    std::size_t tree_size = 0;
    for (std::size_t i = 0; i <= levels; ++i) {
        tree_size += pow(2, i);
    }
    return tree_size;
}
