#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <algorithm>
#include <array>

class mutex {
  public:
    mutex();
    void lock(int thread_index);
    void unlock(int thread_index);

  private:
    std::array<std::atomic<bool>, 2> want;
    std::atomic<int> victim;
};

class tree_mutex {
  public:
    tree_mutex(std::size_t num_threads);
    void lock(std::size_t thread_index);
    void unlock(std::size_t thread_index);

  private:
    std::vector<mutex> mutexes;
    std::size_t levels;
};

mutex::mutex() {
    want[0].store(false);
    want[1].store(false);
    victim.store(0);
}

void mutex::lock(int thread_index) {
    want[thread_index].store(true);
    victim.store(thread_index);
    while (want[1 - thread_index].load() && victim.load() == thread_index) {
        std::this_thread::yield();
    }
}

void mutex::unlock(int thread_index) {
    want[thread_index].store(false);
}

std::size_t realsize(std::size_t size) {
    std::size_t levels = static_cast<std::size_t>(ceil(log2(size)));
    std::size_t realsize = 0;
    for (std::size_t i = 0; i <= levels; ++i) {
        realsize += pow(2, i);
    }
    return realsize;
}

tree_mutex::tree_mutex(std::size_t num_threads) : mutexes(realsize(num_threads)) {
    levels = static_cast<std::size_t>(ceil(log2(num_threads)));
}

void tree_mutex::lock(std::size_t thread_index) {
    int vertex = static_cast<int>(mutexes.size() - pow(2, levels) + thread_index);
    while (vertex > 0) {
        if (vertex % 2 == 0) {
            vertex = (vertex - 2) / 2;
            mutexes[vertex].lock(0);
        } else {
            vertex = (vertex - 1) / 2;
            mutexes[vertex].lock(1);
        }
    }
}

void tree_mutex::unlock(std::size_t thread_index) {
    std::vector<int> path;
    int vertex = static_cast<int>(mutexes.size() - pow(2, levels) + thread_index);
    while (vertex >= 0) {
        path.push_back(vertex);
        if (vertex % 2 == 0) {
            vertex = (vertex - 2) / 2;
        } else {
            vertex = (vertex - 1) / 2;
        }
    }
    for (auto i = path.rbegin(); i < path.rend() - 1; ++i) {
        mutexes[*i].unlock(*(i + 1) % 2);
    }
}
