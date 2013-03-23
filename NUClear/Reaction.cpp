#include "Reaction.h"

namespace NUClear {

    std::atomic<int64_t> Reaction::idSource;

    Reaction::Reaction(std::function<void ()> callback, std::type_index type, reactionId_t parentId)
        : callback(callback), type(type), emitTime(std::chrono::steady_clock::now()), reactionId(++Reaction::idSource), parentId(parentId) {
    }
    
    Reaction::~Reaction() {
        std::cerr << "~Reaction()" << std::endl;
    }

    Reaction::Reaction(Reaction&& other) : 
        type(std::move(other.type)) {
        std::cerr << "Reaction Moved" << std::endl;
        swap(*this, other);
    }    
    
    Reaction& Reaction::operator=(Reaction other) {
        std::cerr << "Reaction Moved operator=" << std::endl;
        swap(*this, other);
        swap(this->type, other.type);
        return *this;
    }

    void swap(Reaction& first, Reaction& second) {
        // If this looks weird to you check out the copy-and-swap idiom:
        // http://stackoverflow.com/a/3279550/203133
        using std::swap; 

        swap(first.callback, second.callback);
        swap(first.emitTime, second.emitTime);
        swap(first.endTime, second.endTime);
        swap(first.reactionId, second.reactionId);
        swap(first.parentId, second.parentId);
    }
}
