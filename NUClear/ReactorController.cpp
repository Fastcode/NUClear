#include "ReactorController.h"

// Instantiates the ReactorControl global.
namespace NUClear {
    ReactorController ReactorControl;

    ReactorController::ReactorController() {
        std::cerr << "ReactorController()" << std::endl;
    }

    ReactorController::~ReactorController() {
        std::cerr << "~ReactorController() - before wait" << std::endl;
        core.kill();
        core.join();
        std::cerr << "~ReactorController() - after wait" << std::endl;
    }
    
    void NUClear::ReactorController::submit(std::unique_ptr<Reaction>&& reaction) {
        core.submit(std::move(reaction));
    }

}
