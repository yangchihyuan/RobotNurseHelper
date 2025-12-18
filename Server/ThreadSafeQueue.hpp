#ifndef __THREADSAFEQUEUE_HPP__
#define __THREADSAFEQUEUE_HPP__
#include <queue>
#include <mutex>

template <typename T>
class ThreadSafeQueue {
public:
    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mtx);
        queue_.push(item);
    }
    bool pop(T& item) {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue_.empty()) {
            return false;
        }
        item = queue_.front();
        queue_.pop();
        return true;
    }
    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue_.size();
    }
    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue_.empty();
    }
    void clear() {
        std::lock_guard<std::mutex> lock(mtx);
        std::queue<T> empty;
        std::swap(queue_, empty);
    }
    T front() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue_.front();
    }
private:
    std::queue<T> queue_;
    std::mutex mtx;
};

#endif // __THREADSAFEQUEUE_HPP__