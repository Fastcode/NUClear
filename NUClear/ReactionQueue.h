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

            ReactionQueue();
            ~ReactionQueue();

            ReactionQueue(ReactionQueue&& other) = delete;
            ReactionQueue& operator=(ReactionQueue&& other) = delete;
            ReactionQueue(const ReactionQueue& other) = delete;
            ReactionQueue& operator=(const ReactionQueue& other) = delete;

            void enqueue(Reaction const& value);
            Reaction dequeue();
    };
}

#endif
