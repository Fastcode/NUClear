#include "Reaction.h"

namespace NUClear {
    
    Reaction::Reaction(std::type_index type) : type(type), emitTime(std::time(nullptr)), eventId(++Reaction::idSource) {
        //TODO make up our event ID from the static source (an atomic int)
    }
    
    Reaction::~Reaction() {
    }
}
