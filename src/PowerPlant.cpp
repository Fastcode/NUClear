/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nuclear_bits/PowerPlant.h"

namespace NUClear {
    
    PowerPlant* PowerPlant::powerplant = nullptr;

    PowerPlant::PowerPlant(Configuration config, int argc, const char *argv[]) :
    configuration(config)
    , threadmaster(*this)
    , cachemaster(*this)
    , reactormaster(*this) {
        
        // Stop people from making more then one powerplant
        if(powerplant) {
            throw std::runtime_error("There is already a powerplant in existance (There should be a single stack allocated PowerPlant");
        }
        
        // Store our static variable
        powerplant = this;
        
        // State that we are setting up
        std::cout << "Building the PowerPlant with " << configuration.threadCount << " thread" << (configuration.threadCount != 1 ? "s" : "") << std::endl;
        
        // Install the chrono extension
        std::cout << "Installing the Chrono extension" << std::endl;
        install<extensions::Chrono>();

        std::cout << "Installing the Networking extension" << std::endl;
        install<extensions::Networking>();

        // Emit our arguments if any.
        if(argc > 0) {
            emit<dsl::Scope::INITIALIZE>(std::make_unique<DataFor<dsl::CommandLineArguments, std::vector<std::string>>>
                                         (std::make_shared<std::vector<std::string>>(argv, argv + argc)));
        }
    }
    
    void PowerPlant::addServiceTask(threading::ThreadWorker::ServiceTask task) {
        threadmaster.serviceTask(task);
    }

    void PowerPlant::start() {
        // ReactorMaster needs to start before we emit initialize so
        // people waiting on Scope::INITIALIZE messages will get
        // them on Trigger<Initialize>
        reactormaster.start();

        emit<dsl::Scope::DIRECT>(std::make_unique<dsl::Startup>());

        threadmaster.start();
    }
    
    void PowerPlant::shutdown() {
        threadmaster.shutdown();
        emit<dsl::Scope::DIRECT>(std::make_unique<dsl::Shutdown>());
        
        // Bye bye powerplant
        powerplant = nullptr;
    }
}
