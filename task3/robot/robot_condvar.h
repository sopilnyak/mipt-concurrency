#include <condition_variable>
#include <string>
#include <mutex>
#include <iostream>

class robot_condvar {
public:
    robot_condvar(int num_steps);
    void step(std::string& leg);

private:
    std::mutex mutex_;
    std::condition_variable left_;
    std::condition_variable right_;
    bool stepped_left_; // if the last step was left

    int num_steps_;
    int step_counter_;
};

robot_condvar::robot_condvar(int num_steps)
        : num_steps_(num_steps),
          step_counter_(0),
          stepped_left_(false) {
}

void robot_condvar::step(std::string& leg) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (step_counter_ < num_steps_) {
        if (leg == "left") {
            right_.wait(lock, [this] { return !stepped_left_; });
            std::cout << "left" << std::endl;
            stepped_left_ = true;
            left_.notify_all();
        } else {
            left_.wait(lock, [this] { return stepped_left_; });
            std::cout << "right" << std::endl;
            stepped_left_ = false;
            right_.notify_all();
        }
        ++step_counter_;
    }
}
