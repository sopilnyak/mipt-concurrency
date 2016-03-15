#include "thread_safe_queue.h"
#include <thread>

const int RANDOM_NUMBERS = 100000;
const int MAX_NUMBER = 10000;
const int POISON_PILL = -1;
const int CONSUMERS = 4;

thread_safe_queue<int> queue(20);

void generate(int number) {
    for (int i = 0; i < number; ++i) {
        int random_number = rand() % MAX_NUMBER;
        queue.enqueue(random_number);
    }
    for (int i = 0; i < CONSUMERS; ++i) {
        queue.enqueue(POISON_PILL);
    }
}

void is_prime() {
    while (true) {
        int number = -1;
        queue.pop(number);
        if (number != POISON_PILL) {
            bool is_prime = true;
            for (int i = 2; i <= number / 2; ++i) {
                if (number % i == 0) {
                    is_prime = false;
                    break;
                }
            }
        } else {
            return;
        }
    }
}

int main() {
    std::thread producer(generate, RANDOM_NUMBERS);
    std::array<std::thread, CONSUMERS> consumer;

    for (int i = 0; i < consumer.size(); ++i) {
        consumer[i] = std::thread(is_prime);
    }
    producer.join();
    for (int i = 0; i < consumer.size(); ++i) {
        consumer[i].join();
    }
    return 0;
}
