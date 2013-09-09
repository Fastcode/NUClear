/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
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

#include "NUClear/PowerPlant.h"

namespace NUClear {

    PowerPlant::PowerPlant(Configuration config, int argc, char *argv[]) :
    configuration(config)
    , threadmaster(this)
    , cachemaster(this)
    , reactormaster(this) {
        // State that we are setting up
        std::cout << "Building the PowerPlant with " << configuration.threadCount << " threads" << std::endl;

        // Install the chrono extension
        std::cout << "Setting up the Chrono extension" << std::endl;
        install<Extensions::Chrono>();

        std::cout << "Setting up the Networking extension" << std::endl;
        install<Extensions::Networking>();

        // Emit our arguments if any.
        if(argc > 0) {
            emit<Internal::CommandTypes::Scope::INITIALIZE>(std::make_unique<Messages::CommandLineArguments>(std::vector<std::string>(argv, argv + argc)));
        }
    }



    void PowerPlant::addServiceTask(Internal::ThreadWorker::ServiceTask task) {
        threadmaster.serviceTask(task);
    }

    void PowerPlant::start() {
        // ReactorMaster needs to start before we emit initialize so
        // people waiting on Scope::INITIALIZE messages will get
        // them on Trigger<Initialize>
        reactormaster.start();

        reactormaster.emit(new Internal::CommandTypes::Initialize());

        threadmaster.start();
    }
    
    void PowerPlant::shutdown() {
        reactormaster.emit(new Internal::CommandTypes::Shutdown());
        threadmaster.shutdown();
    }
}
