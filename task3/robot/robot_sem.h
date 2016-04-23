#include "semaphore.h"
#include <string>
#include <mutex>
#include <atomic>

class robot_sem {
public:
    robot_sem(int num_steps);
    void step(std::string& leg);

private:
    semaphore left_;
    semaphore right_;

    int num_steps_;
    int step_counter_;
};

robot_sem::robot_sem(int num_steps)
        : num_steps_(num_steps),
          step_counter_(0) {
    right_.signal();
}

void robot_sem::step(std::string& leg) {
    while (step_counter_ < num_steps_) {
        if (leg == "left") {
            right_.wait();
            std::cout << "left" << std::endl;
            left_.signal();
        } else {
            left_.wait();
            std::cout << "right" << std::endl;
            right_.signal();
        }
        ++step_counter_;
    }
}
