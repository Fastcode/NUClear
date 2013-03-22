#include "ReactionQueue.h"

namespace NUClear {

    void ReactionQueue::enqueue(Reaction const& value) {
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            queue.push_front(value);
            std::cout << value.reactionId << std::endl;
        }
        this->condition.notify_one();
    }
    
    Reaction ReactionQueue::dequeue() {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->condition.wait(lock, [this]() {
            std::cerr << "Queue empty: " << !this->queue.empty() << std::endl;
            return !this->queue.empty();
        });
        Reaction r = queue.back();
        std::cerr << "Reaction type name: " << r.type.name() << std::endl;
        queue.pop_back();
        return r;
    }
}
