#include "NUClear/Environment.h"
#include "NUClear/PowerPlant.h"
namespace NUClear {
    Environment::Environment(PowerPlant* powerPlant, LogLevel logLevel) :
        powerPlant(powerPlant),
        logLevel(logLevel)
    {}
}
