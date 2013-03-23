#include "ReactorController.h"

// Instantiates the ReactorControl global.
namespace NUClear {
    ReactorController ReactorControl;

    ReactorController::ReactorController() {
        std::cerr << "ReactorController()" << std::endl;
    }

    ReactorController::~ReactorController() {
        std::cerr << "~ReactorController()" << std::endl;
    }
    
    void NUClear::ReactorController::submit(std::unique_ptr<Reaction>&& reaction) {
        core.submit(std::move(reaction));
    }

    void NUClear::ReactorController::killjoin() {
        std::cerr << "Before Killjoin" << std::endl;
        core.kill();
        core.join();
        std::cerr << "After Killjoin" << std::endl;
    }

}
