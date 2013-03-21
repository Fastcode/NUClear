#include "Reaction.h"

namespace NUClear {

    std::atomic<int64_t> Reaction::idSource;
    
    Reaction::Reaction(std::type_index type) : type(type), emitTime(std::time(nullptr)), eventId(++Reaction::idSource) {
        //TODO make up our event ID from the static source (an atomic int)
    }
    
    Reaction::~Reaction() {
    }
}
