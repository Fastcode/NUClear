/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_POWERPLANT_HPP
#define NUCLEAR_POWERPLANT_HPP

#include <memory>
#include <set>
#include <thread>
#include <vector>
#include <queue>
#include <typeindex>
#include <mutex>
#include <map>
#include <iostream>
#include <string>
#include <sstream>

// Utilities
#include "nuclear_bits/util/unpack.hpp"
#include "nuclear_bits/util/demangle.hpp"
#include "nuclear_bits/util/FunctionFusion.hpp"

#include "nuclear_bits/LogLevel.hpp"
#include "nuclear_bits/threading/TaskScheduler.hpp"
#include "nuclear_bits/message/LogMessage.hpp"

namespace NUClear {

    // Forward declare reactor
    class Reactor;

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
            /// @brief default to the amount of hardware concurrency (or 2) threads
            Configuration() : threadCount(std::thread::hardware_concurrency() == 0 ? 2 : std::thread::hardware_concurrency()) {}

            /// @brief The number of threads the system will use
            size_t threadCount;
        };

        /// @brief Holds the configuration information for this PowerPlant (such as number of pool threads)
        const Configuration configuration;


        // There can only be one powerplant, so this is it
        static PowerPlant* powerplant;

        /**
         * @brief
         *  Constructs a PowerPlant with the given configuration and provides access
         *  to argv for all reactors.
         *
         * @details
         *  If PowerPlant is constructed with argc and argv then a CommandLineArguments
         *  message will be emitted and available to all reactors.
         */
        PowerPlant(Configuration config = Configuration(), int argc = 0, const char *argv[] = nullptr);
        PowerPlant(const PowerPlant&) = delete;
        ~PowerPlant();

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
         * TODO document
         */
        bool running();

        /**
         * TODO document
         */
        void onStartup(std::function<void ()>&& func);

        /**
         * TODO document
         */
        void addThreadTask(std::function<void ()>&& task);

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
         * @brief Submits a new task to the ThreadPool to be queued and then executed.
         *
         * @param task The Reaction task to be executed in the thread pool
         */
        void submit(std::unique_ptr<threading::ReactionTask>&& task);

        /**
         * @brief Submits a new task to the main threads thread pool to be queued and then executed.
         *
         * @param task The Reaction task to be executed in the thread pool
         */
        void submitMain(std::unique_ptr<threading::ReactionTask>&& task);

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
        static void log(TArgs&&... args);

        /**
         * @brief Emits data to the system and routes it to the other systems that use it.
         *
         * @details
         *  TODO
         *
         * @tparam TData    The type of the data that we are emitting
         *
         * @param data The data we are emitting
         */
        template <typename TData>
        void emit(std::unique_ptr<TData>&& data);
        template <typename TData>
        void emit(std::unique_ptr<TData>& data);

        /**
         * @brief Emits data to the system and routes it to the other systems that use it.
         *
         * @details
         *  TODO
         *
         * @tparam THandlers        the first handler to use for this emit
         * @tparam TFirstHandler    the remaining handlers to use for this emit
         * @tparam TData            the type of the data that we are emitting
         * @tparam TArgs            the additional arguments that will be provided to the handlers
         *
         * @param data The data we are emitting
         */
        template <template <typename> class TFirstHandler, template <typename> class... THandlers, typename TData, typename... TArgs>
        void emit_shared(std::shared_ptr<TData>&& data, TArgs&&... args);
        template <template <typename> class TFirstHandler, template <typename> class... THandlers, typename TData, typename... TArgs>
        void emit(std::unique_ptr<TData>&& data, TArgs&&... args);
        template <template <typename> class TFirstHandler, template <typename> class... THandlers, typename TData, typename... TArgs>
        void emit(std::unique_ptr<TData>& data, TArgs&&... args);

    private:
        /// @brief A list of tasks that must be run when the powerplant starts up
        std::vector<std::function<void ()>> tasks;
        /// @brief A vector of the running threads in the system
        std::vector<std::unique_ptr<std::thread>> threads;
        /// @brief Our TaskScheduler that handles distributing task to the pool threads
        threading::TaskScheduler scheduler;
        /// @brief Our TaskScheduler that handles distributing tasks to the main thread
        threading::TaskScheduler mainThreadScheduler;
        /// @brief Our vector of Reactors, will get destructed when this vector is
        std::vector<std::unique_ptr<NUClear::Reactor>> reactors;


        std::vector<std::function<void ()>> startupTasks;
        volatile bool isRunning = false;
    };

    // This free floating log function can be called from anywhere and will use the singleton PowerPlant
    template <enum LogLevel level = NUClear::DEBUG, typename... TArgs>
    void log(TArgs&&... args) {
        PowerPlant::log<level>(std::forward<TArgs>(args)...);
    }

}  // namespace NUClear

// Include our Reactor.h first as the tight coupling between powerplant and reactor requires a specific include order
#include "nuclear_bits/Reactor.hpp"

// Emit types
#include "nuclear_bits/dsl/word/emit/Local.hpp"
#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/dsl/word/emit/Initialize.hpp"

// Built in smart types
#include "nuclear_bits/message/CommandLineArguments.hpp"
#include "nuclear_bits/message/NetworkConfiguration.hpp"
#include "nuclear_bits/message/NetworkEvent.hpp"

// Header which stops reaction statisitcs messages from triggering themselves
#include "nuclear_bits/dsl/operation/ReactionStatisticsDeloop.hpp"

// Include all of our implementation files (which use the previously included reactor.h)
#include "nuclear_bits/PowerPlant.ipp"

#endif  // NUCLEAR_POWERPLANT_HPP
