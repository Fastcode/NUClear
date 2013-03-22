#include "Reaction.h"

namespace NUClear {

    std::atomic<int64_t> Reaction::idSource;
    
    Reaction::Reaction(std::function<void ()> callback, std::type_index type, reactionId_t parentId)
    : callback(callback), type(type), emitTime(std::chrono::steady_clock::now()), reactionId(++Reaction::idSource), parentId(parentId) {
    }
    
    Reaction::~Reaction() {
        std::cerr << "~Reaction()" << std::endl;
    }
}
