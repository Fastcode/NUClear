#include "ReactorController.h"

// Instantiates the ReactorControl global.
namespace NUClear {
    ReactorController ReactorControl;
    
    void NUClear::ReactorController::submit(Reaction r) {
        core.submit(r);
    }
}
