/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <typeindex>
#include <vector>

// Utilities
#include "nuclear_bits/util/FunctionFusion.hpp"
#include "nuclear_bits/util/demangle.hpp"
#include "nuclear_bits/util/unpack.hpp"

#include "nuclear_bits/LogLevel.hpp"
#include "nuclear_bits/message/LogMessage.hpp"
#include "nuclear_bits/threading/TaskScheduler.hpp"

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
     */
    struct Configuration {
        /// @brief default to the amount of hardware concurrency (or 2) threads
        Configuration()
            : thread_count(std::thread::hardware_concurrency() == 0 ? 2 : std::thread::hardware_concurrency()) {}

        /// @brief The number of threads the system will use
        size_t thread_count;
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
    PowerPlant(Configuration config = Configuration(), int argc = 0, const char* argv[] = nullptr);
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
    void on_startup(std::function<void()>&& func);

    /**
     * TODO document
     */
    void add_thread_task(std::function<void()>&& task);

    /**
     * @brief Installs a reactor of a particular type to the system.
     *
     * @details
     *  This function constructs a new Reactor of the template type.
     *  It passes through the specified LogLevel
     *  in the environment of that reactor so that it can be used to filter logs.
     *
     * @tparam T        The type of the reactor to build and install
     * @tparam level    The initial logging level for this reactor to use
     */
    template <typename T, enum LogLevel level = DEBUG>
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
    void submit_main(std::unique_ptr<threading::ReactionTask>&& task);

    /**
     * @brief Log a message through NUClear's system.
     *
     * @details
     *  Logs a message through the system so the various log handlers
     *  can access it.
     *
     * @tparam level     The level to log at (defaults to DEBUG)
     * @tparam Arguments The types of the arguments we are logging
     *
     * @param args The arguments we are logging
     */
    template <enum LogLevel level, typename... Arguments>
    static void log(Arguments&&... args);

    /**
     * @brief Emits data to the system and routes it to the other systems that use it.
     *
     * @details
     *  TODO
     *
     * @tparam T    The type of the data that we are emitting
     *
     * @param data The data we are emitting
     */
    template <typename T>
    void emit(std::unique_ptr<T>&& data);
    template <typename T>
    void emit(std::unique_ptr<T>& data);

    /**
     * @brief Emits data to the system and routes it to the other systems that use it.
     *
     * @details
     *  TODO
     *
     * @tparam First        the first handler to use for this emit
     * @tparam Remainder    the remaining handlers to use for this emit
     * @tparam T            the type of the data that we are emitting
     * @tparam Arguments        the additional arguments that will be provided to the handlers
     *
     * @param data The data we are emitting
     */
    template <template <typename> class First,
              template <typename> class... Remainder,
              typename T,
              typename... Arguments>
    void emit_shared(std::shared_ptr<T>&& data, Arguments&&... args);

    template <template <typename> class First,
              template <typename> class... Remainder,
              typename T,
              typename... Arguments>
    void emit(std::unique_ptr<T>&& data, Arguments&&... args);

    template <template <typename> class First,
              template <typename> class... Remainder,
              typename T,
              typename... Arguments>
    void emit(std::unique_ptr<T>& data, Arguments&&... args);

private:
    /// @brief A list of tasks that must be run when the powerplant starts up
    std::vector<std::function<void()>> tasks;
    /// @brief A vector of the running threads in the system
    std::vector<std::unique_ptr<std::thread>> threads;
    /// @brief Our TaskScheduler that handles distributing task to the pool threads
    threading::TaskScheduler scheduler;
    /// @brief Our TaskScheduler that handles distributing tasks to the main thread
    threading::TaskScheduler main_thread_scheduler;
    /// @brief Our vector of Reactors, will get destructed when this vector is
    std::vector<std::unique_ptr<NUClear::Reactor>> reactors;
    /// @brief Tasks that will be run during the startup process
    std::vector<std::function<void()>> startup_tasks;
    /// @brief True if the powerplant is running
    volatile bool is_running = false;
};

// This free floating log function can be called from anywhere and will use the singleton PowerPlant
template <enum LogLevel level = NUClear::DEBUG, typename... Arguments>
void log(Arguments&&... args) {
    PowerPlant::log<level>(std::forward<Arguments>(args)...);
}

}  // namespace NUClear

// Include our Reactor.h first as the tight coupling between powerplant and reactor requires a specific include order
#include "nuclear_bits/Reactor.hpp"

// Emit types
#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/dsl/word/emit/Initialise.hpp"
#include "nuclear_bits/dsl/word/emit/Local.hpp"

// Built in smart types
#include "nuclear_bits/message/CommandLineArguments.hpp"
#include "nuclear_bits/message/NetworkConfiguration.hpp"
#include "nuclear_bits/message/NetworkEvent.hpp"

// Header which stops reaction statisitcs messages from triggering themselves
#include "nuclear_bits/dsl/operation/ReactionStatisticsDeloop.hpp"

// Include all of our implementation files (which use the previously included reactor.h)
#include "nuclear_bits/PowerPlant.ipp"

#endif  // NUCLEAR_POWERPLANT_HPP
