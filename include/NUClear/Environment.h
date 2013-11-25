#ifndef NUCLEAR_ENVIRONMENT_H
#define NUCLEAR_ENVIRONMENT_H
#include "NUClear/LogLevel.h"
#include "NUClear/ForwardDeclarations.h"
namespace NUClear {
    class Environment {
        public:
            Environment(PowerPlant* powerPlant, LogLevel logLevel = LogLevel::INFO);

        private:
            friend class PowerPlant;
            friend class Reactor;

            PowerPlant* powerPlant;
            LogLevel logLevel;
    };
}
#endif
