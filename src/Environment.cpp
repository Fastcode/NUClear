#include "nuclear_bits/Environment.h"
#include "nuclear_bits/PowerPlant.h"
namespace NUClear {
    Environment::Environment(PowerPlant* powerPlant, LogLevel logLevel) :
        powerPlant(powerPlant),
        logLevel(logLevel)
    {}
}
