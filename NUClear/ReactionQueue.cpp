#include "ReactionQueue.h"
#include <iostream>

namespace NUClear {

    void ReactionQueue::enqueue(Reaction const& value) {
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            queue.push_front(value);
        }
        this->condition.notify_one();
    }
    
    Reaction ReactionQueue::dequeue() {
        std::cout << queue.empty() << std::endl;
        std::unique_lock<std::mutex> lock(this->mutex);
        this->condition.wait(lock, [=]{ return !this->queue.empty(); });
        Reaction r = queue.back();
        queue.pop_back();
        return r;
    }
}