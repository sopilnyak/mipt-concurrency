#include "CTreeMutex.h"
#include <thread>
#include <atomic>
#include <algorithm>

CTreeMutex::CTreeMutex(std::size_t numThreads) : numThreads(numThreads) {
    want.resize(numThreads, 0);
    turn.resize(numThreads - 1, 0);
}

void CTreeMutex::lock(std::size_t treeIndex) {
    for (std::size_t i = 0; i < numThreads - 1; ++i) {
        want[treeIndex] = i + 1;
        turn[i] = treeIndex;
        while (exists(treeIndex, i) && turn[i] == treeIndex) {
            std::this_thread::yield(); // wait
        }
    }
}

bool CTreeMutex::exists(std::size_t treeIndex, std::size_t index) {
    auto check = [index](std::size_t value) { return value >= index + 1; };
    return std::any_of(want.begin(), want.begin() + treeIndex, check)
           || std::any_of(want.begin() + treeIndex + 1, want.end(), check);
}

void CTreeMutex::unlock(std::size_t treeIndex) {
    want[treeIndex] = 0;
}
