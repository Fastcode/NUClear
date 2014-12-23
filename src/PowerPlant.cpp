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

    PowerPlant::PowerPlant(Configuration config, int argc, const char *argv[])
      : configuration(config)
      , threadmaster(*this)
      , reactormaster(*this) {
        
        // Stop people from making more then one powerplant
        if(powerplant) {
            throw std::runtime_error("There is already a powerplant in existence (There should be a single PowerPlant)");
        }
        
        // Store our static variable
        powerplant = this;
        
        // State that we are setting up
        std::cout << "Building the PowerPlant with " << configuration.threadCount << " thread" << (configuration.threadCount != 1 ? "s" : "") << std::endl;
        
        // Emit our arguments if any.
        auto args = std::make_unique<message::CommandLineArguments>();
        
        for (int i = 0; i < argc; ++i) {
            args->emplace_back(argv[i]);
        }
        
        emit<dsl::word::emit::Initialize>(std::move(args));
    }
    
    void PowerPlant::onStartup(std::function<void ()>&& func) {
        
        startupTasks.push_back(func);
    }

    void PowerPlant::start() {

        // Direct emit startup event
        emit<dsl::word::emit::Direct>(std::make_unique<dsl::word::Startup>());
        
        // Run all our Initialise scope tasks
        for(auto&& func : startupTasks) {
            func();
        }
        
        threadmaster.start();
    }
    
    void PowerPlant::submit(std::unique_ptr<threading::ReactionTask>&& task) {
        threadmaster.submit(std::forward<std::unique_ptr<threading::ReactionTask>>(task));
    }
    
    void PowerPlant::shutdown() {
        
        // Emit our shutdown event
        emit(std::make_unique<dsl::word::Shutdown>());
        
        // Shutdown the threads
        threadmaster.shutdown();
        
        // Bye bye powerplant
        powerplant = nullptr;
    }
}
