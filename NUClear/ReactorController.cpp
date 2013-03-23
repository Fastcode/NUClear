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

    /*void NUClear::ReactorController::stop() {
        std::cerr << "Before reactorcontroller::stop" << std::endl;
        //core.stop();
        std::cerr << "After reactorcontroller::stop" << std::endl;
    }*/

    void NUClear::ReactorController::waitForThreadCompletion() {
        core.waitForThreadCompletion();
    }

}
