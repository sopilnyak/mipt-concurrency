#include <iostream>
#include <vector>

class CTreeMutex {
private:
    std::size_t numThreads;
    std::vector<std::size_t> want;
    std::vector<std::size_t> turn;
    bool exists(std::size_t treeIndex, std::size_t index);

public:
    CTreeMutex(std::size_t numThreads);
    void lock(std::size_t treeIndex);
    void unlock(std::size_t treeIndex);
};

