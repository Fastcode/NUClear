#include "Reactor.h"
namespace NUClear {
    Reactor::Reactor(ReactorController& reactorController) :
        reactorController(reactorController) {
    }

    Reactor::~Reactor() {}
}
