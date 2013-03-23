#include "ReactorController.h"

// Instantiates the ReactorControl global.
namespace NUClear {
    ReactorController ReactorControl;
    
    void NUClear::ReactorController::submit(std::unique_ptr<Reaction>&& reaction) {
        core.submit(std::move(reaction));
    }
}
