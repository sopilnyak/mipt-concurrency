#include <memory>
#include <future>

template <class R>
class task {
public:
    task();
    task(std::shared_ptr<std::promise<R>> promise, std::function<R()> function);
    void run() const;
    bool is_poison_pill() const;
    void make_poison_pill();

private:
    bool is_poison_pill_;
    std::function<R()> function_;
    std::shared_ptr<std::promise<R>> promise_;
};

template <class R>
task<R>::task()
        : is_poison_pill_(false) {
}

template <class R>
task<R>::task(std::shared_ptr<std::promise<R>> promise, std::function<R()> function)
        : is_poison_pill_(false), function_(function), promise_(promise) {
}

template <class R>
void task<R>::run() const {
    R returned;
    try {
        returned = function_();
    } catch (...) {
        promise_->set_exception(std::current_exception());
        return;
    }
    promise_->set_value(returned);
}

template <class R>
bool task<R>::is_poison_pill() const {
    return is_poison_pill_;
}

template <class R>
void task<R>::make_poison_pill() {
    is_poison_pill_ = true;
}
