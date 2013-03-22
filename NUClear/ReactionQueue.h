#ifndef NUCLEAR_REACTIONQUEUE_H
#define NUCLEAR_REACTIONQUEUE_H

#include <deque>
#include <condition_variable>
#include <mutex>
#include "Reaction.h"

namespace NUClear {
    
    class ReactionQueue
    {
        private:
            std::mutex mutex;
            std::condition_variable condition;
            std::deque<Reaction> queue;
        public:
            // Deleted Constructors
            ReactionQueue(const ReactionQueue& other) = delete;
            operator=(const ReactionQueue& other) = delete;

            void enqueue(Reaction const& value);
            Reaction dequeue();
    };
}

#endif
