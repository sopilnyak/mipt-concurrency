#include "robot_condvar.h"
#include "robot_sem.h"
#include <thread>

const int NUM_STEPS = 30;

template <typename Robot>
void test() {
    Robot robot(NUM_STEPS);
    std::string left = "left";
    std::string right = "right";

    auto step = [&robot] (std::string leg) {
        robot.step(leg);
    };

    std::thread left_leg(step, left);
    std::thread right_leg(step, right);

    left_leg.join();
    right_leg.join();
}

int main() {
    test<robot_condvar>();
    test<robot_sem>();
    return 0;
}
