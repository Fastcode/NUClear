#ifndef NUCLEAR_REACTORTASKQUEUE_H
#define NUCLEAR_REACTORTASKQUEUE_H

#include <mutex>
#include <condition_variable>
#include <deque>

namespace NUClear {
    template <typename T>
    class ReactorTaskQueue {
        private:
            std::mutex mutex;
            std::condition_variable condition;
            std::deque<T> queue;
        public:
            ReactorTaskQueue();
            ~ReactorTaskQueue();
            void submit(T const& value) {
                // This scope is here such that when notify is called, the lock has been released
                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    queue.push_front(value);
                }
                this->condition.notify_one();
            }
            T getTask() {
                std::unique_lock<std::mutex> lock(this->mutex);
                this->condition.wait(lock, [=]{ return !this->queue.empty(); });
                T rc(std::move(this->queue.back()));
                this->queue.pop_back();
                return rc;
            }
    };
}

#endif