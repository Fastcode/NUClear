#include "Reactor.h"

namespace NUClear {
    Reactor::Reactor() : reactorControl(ReactorControl) {
    }

    Reactor::~Reactor() {
        std::cerr << "~Reactor()" << std::endl;
    }
}
