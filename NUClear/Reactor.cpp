#include "Reactor.h"
namespace NUClear {
    Reactor::Reactor(ReactorController& reactorController) :
        reactorController(reactorController) {
    }
    
    void Reactor::buildOptionsImpl(Internal::ReactionOptions& options, Single* /*placeholder*/) {
        options.m_single = true;
    }

    Reactor::~Reactor() {}
}
