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

#ifndef NUCLEAR_POWERPLANT_H
#define NUCLEAR_POWERPLANT_H

#include <memory>
#include <set>
#include <thread>
#include <vector>
#include <queue>
#include <typeindex>
#include <mutex>
#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <sstream>

// Forward declarations
#include "nuclear_bits/ForwardDeclarations.h"

// Utilities
#include "nuclear_bits/util/unpack.h"

// Emit types
#include "nuclear_bits/dsl/word/LocalEmit.h"
#include "nuclear_bits/dsl/word/DirectEmit.h"
#include "nuclear_bits/threading/ThreadWorker.h"
#include "nuclear_bits/threading/TaskScheduler.h"
#include "nuclear_bits/LogLevel.h"
#include "nuclear_bits/LogMessage.h"

// Patch for std::make_unique in c++11 (should be fixed in c++14)
#if __cplusplus == 201103L
namespace std {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif

namespace NUClear {
    
    /**
     * @brief The PowerPlant is the core of a NUClear system. It holds all Reactors in it and manages their communications.
     *
     * @details
     *  At the centre of every NUClear system is a PowerPlant. A PowerPlant contains all of the reactors that are
     *  used within the system and sets up their reactions. It is also responsible for storing information between
     *  reactions and ensuring that all threading is handled appropriately.
     *
     * @author Trent Houliston
     * @author Jake Woods
     */
    class PowerPlant {
        // Reactors and PowerPlants are very tightly linked
        friend class Reactor;
        
    public:
        /**
         * @brief This class holds the configuration for a PowerPlant.
         *
         * @details
         *  It configures the number of threads that will be in the PowerPlants thread pool
         *
         * @author Trent Houliston
         */
        struct Configuration {
            /// @brief default to 4 threads
            Configuration() : threadCount(4) {}
            
            /// @brief The number of threads the system will use
            unsigned threadCount;
        };
        
    private:
        // There can only be one powerplant, so this is it
        static PowerPlant* powerplant;
        
        /**
         * @brief The base master class is used as a base for all of the other masters.
         *
         * @details
         *  A master is a segment of the PowerPlant that is responsible for a ptaicular role within the system.
         *  They all have the same requirement to store a reference to the PowerPlant that they are a part of.
         *
         * @author Jake Woods
         */
        class BaseMaster {
        public:
            /// @brief Construct a new BaseMaster with our PowerPlant as context
            BaseMaster(PowerPlant& parent) : parent(parent) {}
        protected:
            /// @brief The PowerPlant that this belongs to
            PowerPlant& parent;
        };
        
        /**
         * @brief The ThreadMaster class is responsible for managing the thread pool and any service threads needed.
         *
         * @details
         *  The ThreadMaster holds all of the threads in the system so that it can run their start and kill methods.
         *  This allows the system to perform a graceful termination in the event of a system shutdown.
         *  It is also responsible for holding the Thread Pool for reactions, and the scheduler that allocates them.
         */
        class ThreadMaster : public BaseMaster {
            
        public:
            static __thread const threading::ReactionTask* currentTask;

            /// @brief Construct a new ThreadMaster with our PowerPlant as context
            ThreadMaster(PowerPlant& parent);
            
            /**
             * @brief Starts up the ThreadMaster initiating all service threads and pool threads.
             *
             * @details
             *  This will start up the entire system, beginning all threads and running the needed code. It will
             *  block the thread that calls it until the system is shut down. This method should only be called
             *  from the main thread. This will prevent the main thread ending which deletes all statics (which
             *  are important for us).
             */
            void start();
            
            /**
             * @brief
             *  Shuts down the ThreadMaster, it will terminate all of the service and pool threads, and then
             *  releases the main thread to terminate the system.
             */
            void shutdown();
            
            /**
             * @brief Submits a new task to the ThreadPool to be queued and then executed.
             *
             * @param task The Reactor task to be executed in the thread pool
             */
            void submit(std::unique_ptr<threading::ReactionTask>&& task);
            
        private:
            /// @brief A vector of the threads in the system
            std::vector<std::unique_ptr<threading::ThreadWorker>> threads;
            /// @brief Our TaskScheduler that handles distributing task to the pool threads
            threading::TaskScheduler scheduler;
        };
        
        /**
         * @brief The reactor master is responsible for holding all Reactors, as well as reactions.
         *  It also hands out tasks and collects data for the cache master.
         *
         * @attention
         *  This ReactorMaster uses static variables to enhance its speed, this means that you cannot have more then
         *  one PowerPlant in an executable without resolving this.
         */
        class ReactorMaster : public BaseMaster {
        public:
            /**
             * @brief Constructs a new ReactorMaster which is held in the PowerPlant that created it.
             */
            ReactorMaster(PowerPlant& parent) : PowerPlant::BaseMaster(parent) {}
            
            /**
             * @brief Builds and installs a reactor of the passed type.
             *
             * @tparam TReactor The type of the reactor we are installing
             */
            template <typename TReactor, enum LogLevel level = DEBUG>
            void install();
            
        private:
            
            /// @brief Our vector of Reactors, will get destructed when this vector is
            std::vector<std::unique_ptr<NUClear::Reactor>> reactors;
        };
        
    public:
        
        /// @brief Holds the configuration information for this PowerPlant (such as number of pool threads)
        const Configuration configuration;
        
    protected:
        /// @brief The ThreadMaster instance for this PowerPlant.
        ThreadMaster threadmaster;
        /// @brief The ReactorMaster instance for this PowerPlant.
        ReactorMaster reactormaster;
    public:
        /**
         * @brief
         *  Constructs a PowerPlant with the given configuration and provides access
         *  to argv for all reactors.
         *
         * @details
         *  If PowerPlant is constructed with argv and argv then a CommandLineArguments
         *  message will be emitted and available to all reactors.
         */
        PowerPlant(Configuration config = Configuration(), int argc = 0, const char *argv[] = nullptr);
        
        /**
         * @brief Starts up this PowerPlants components in order and begins it running.
         *
         * @details
         *  Starts up the PowerPlant instance and starts all the pool threads. This
         *  method is blocking and will release when the PowerPlant shuts down.
         *  It should only be called from the main thread so that statics are not
         *  destructed.
         */
        void start();
        
        /**
         * @brief Shuts down the PowerPlant, tells all component threads to terminate,
         *  Then releases the main thread.
         */
        void shutdown();
        
        /**
         * @brief Installs a reactor of a particular type to the system.
         *
         * @details
         *  This function creates a new Reactor of the type that is passed in
         *  TReactor and constructs it. It passes through the specified LogLevel
         *  in the environment of that reactor so that it can be used to filter logs.
         *
         * @tparam TReactor The type of the reactor to build and install
         * @tparam level    The Logging level for this reactor to use
         */
        template <typename TReactor, enum LogLevel level = DEBUG>
        void install();
        
        /**
         * @brief Log a message through NUClear's system.
         *
         * @details
         *  Logs a message through the system so the various log handlers
         *  can access it.
         *
         * @tparam level The level to log at (defaults to DEBUG)
         * @tparam TArgs The types of the arguments we are logging
         *
         * @param args The arguments we are logging
         */
        template <enum LogLevel level, typename... TArgs>
        static void log(TArgs... args);
        
        /**
         * @brief Emits data to the system and routes it to the other systems that use it.
         *
         * @details
         *  TODO
         *
         * @tparam THandlers    The handlers to use for this emit
         * @tparam TData        The type of the data that we are emitting
         *
         * @param data The data we are emitting
         */
        template <typename TData>
        void emit(std::unique_ptr<TData>&& data);
        
        /**
         * @brief Emits data to the system and routes it to the other systems that use it.
         *
         * @details
         *  TODO
         *
         * @tparam THandlers    The handlers to use for this emit
         * @tparam TData        The type of the data that we are emitting
         *
         * @param data The data we are emitting
         */
        template <typename TFirstHandler, typename... THandlers, typename TData>
        void emit(std::unique_ptr<TData>&& data);
    };
    
    // This free floating log function can be called from anywhere and will use the singleton PowerPlant
    template <enum LogLevel level = NUClear::DEBUG, typename... TArgs>
    void log(TArgs... args) {
        PowerPlant::log<level>(std::forward<TArgs>(args)...);
    }
    
}

// Include our Reactor.h first as the tight coupling between powerplant and reactor requires a specific include order
#include "nuclear_bits/Reactor.h"

// Include all of our implementation files (which use the previously included reactor.h)
#include "nuclear_bits/PowerPlant.ipp"
#include "nuclear_bits/ReactorMaster.ipp"

#endif

