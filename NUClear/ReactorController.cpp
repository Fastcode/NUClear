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
    
    void ReactorController::submit(std::unique_ptr<Reaction>&& reaction) {
        core.submit(std::move(reaction));
    }
    
    void ReactorController::addEvery(std::chrono::nanoseconds time, std::function<void ()> callback) {
        timeEmitter.add(time, callback);
    }

    void ReactorController::shutdown() {
        std::cerr << "Before reactorcontroller::stop" << std::endl;
        core.shutdown();
        std::cerr << "After reactorcontroller::stop" << std::endl;
    }

    void ReactorController::waitForThreadCompletion() {
        core.waitForThreadCompletion();
    }

}
