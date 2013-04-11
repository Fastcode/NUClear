#include "ReactorController.h"
namespace NUClear {
    ReactorController::ReactorController() :
        chronomaster(this)
        , reactormaster(this)
        , threadmaster(this) {}

    void ReactorController::start() {
    }
}
