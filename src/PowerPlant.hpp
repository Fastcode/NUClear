/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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

#ifndef NUCLEAR_POWER_PLANT_HPP
#define NUCLEAR_POWER_PLANT_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

// Utilities
#include "Configuration.hpp"
#include "Environment.hpp"
#include "LogLevel.hpp"
#include "id.hpp"
#include "threading/ReactionTask.hpp"
#include "threading/scheduler/Scheduler.hpp"
#include "util/FunctionFusion.hpp"
#include "util/demangle.hpp"

namespace NUClear {

// Forward declarations
class Reactor;
namespace util {
    struct ThreadPoolDescriptor;
}  // namespace util
namespace dsl {
    namespace word {
        namespace emit {
            template <typename T>
            struct Local;
        }  // namespace emit
    }  // namespace word
}  // namespace dsl

/**
 * The PowerPlant is the core of a NUClear system. It holds all Reactors in it and manages their communications.
 *
 * At the centre of every NUClear system is a PowerPlant.
 * A PowerPlant contains all of the reactors that are used within the system and sets up their reactions.
 * It is also responsible for storing information between reactions and ensuring threading is handled appropriately.
 */
class PowerPlant {
    // Reactors and PowerPlants are very tightly linked
    friend class Reactor;

    /**
     * This is our Function Fusion wrapper class that allows it to call emit functions
     *
     * @tparam Handler The emit handler that we are wrapping for
     */
    template <typename Handler>
    struct EmitCaller {
        template <typename... Arguments>
        static auto call(Arguments&&... args)
            // THIS IS VERY IMPORTANT, the return type must be dependent on the function call
            // otherwise it won't check it's valid in SFINAE (the comma operator does it again!)
            -> decltype(Handler::emit(std::forward<Arguments>(args)...), true) {
            Handler::emit(std::forward<Arguments>(args)...);
            return true;
        }
    };

public:
    // There can only be one powerplant, so this is it
    static PowerPlant* powerplant;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    /**
     * Constructs a PowerPlant with the given configuration and provides access to argv for all reactors.
     *
     * Arguments passed to this function will be emitted as a CommandLineArguments message.
     *
     * @param config The PowerPlant's configuration
     * @param argc   The number of command line arguments
     * @param argv   The command line argument strings
     */
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    PowerPlant(Configuration config = Configuration(), int argc = 0, const char* argv[] = nullptr);
    ~PowerPlant();

    // There can only be one!
    PowerPlant(const PowerPlant& other)             = delete;
    PowerPlant(const PowerPlant&& other)            = delete;
    PowerPlant& operator=(const PowerPlant& other)  = delete;
    PowerPlant& operator=(const PowerPlant&& other) = delete;

    /**
     * Starts up the PowerPlant's components in order and begins it running.
     *
     * Starts up the PowerPlant instance and starts all the pool threads.
     * This method is blocking and will release when the PowerPlant shuts down.
     * It should only be called from the main thread so that statics are not destructed.
     */
    void start();

    /**
     * Shuts down the PowerPlant, tells all component threads to terminate and waits for them to finish.
     *
     * @param force If true, the PowerPlant will shutdown immediately without waiting for tasks to finish
     */
    void shutdown(bool force = false);

    /**
     * Installs a reactor of a particular type to the system.
     *
     * This function constructs a new Reactor of the template type.
     * It passes the specified LogLevel in the environment of that reactor so that it can be used to filter logs.
     *
     * @tparam T     The type of the reactor to build and install
     * @tparam Args  The types of the extra arguments to pass to the reactor constructor
     * @tparam level The initial logging level for this reactor to use
     *
     * @param arg Extra arguments to pass to the reactor constructor
     *
     * @return A reference to the installed reactor
     */
    template <typename T, typename... Args>
    T& install(Args&&... args) {

        // Make sure that the class that we received is a reactor
        static_assert(std::is_base_of<Reactor, T>::value, "You must install Reactors");

        // The reactor constructor should handle subscribing to events
        reactors.push_back(std::make_unique<T>(std::make_unique<Environment>(*this, util::demangle(typeid(T).name())),
                                               std::forward<Args>(args)...));

        return static_cast<T&>(*reactors.back());
    }

    /**
     * Adds an idle task to the task scheduler.
     *
     * This function adds an idle task to the task scheduler, which will be executed when the thread pool associated
     * with the given `pool_id` has no other tasks to execute.
     * The `task` parameter is a Reaction from which a task will be submitted when the pool is idle.
     *
     * @param reaction        The reaction to be executed when idle
     * @param pool_descriptor The descriptor for the thread pool to test for idle or nullptr for all pools
     */
    void add_idle_task(const std::shared_ptr<threading::Reaction>& reaction,
                       const std::shared_ptr<const util::ThreadPoolDescriptor>& pool_descriptor = nullptr);

    /**
     * Removes an idle task from the task scheduler.
     *
     * This function removes an idle task from the task scheduler. The `id` and `pool_id` parameters are used to
     * identify the idle task to be removed.
     *
     * @param id              The reaction id of the task to be removed
     * @param pool_descriptor The descriptor for the thread pool to test for idle
     */
    void remove_idle_task(const NUClear::id_t& id,
                          const std::shared_ptr<const util::ThreadPoolDescriptor>& pool_descriptor = nullptr);

    /**
     * Submits a new task to the ThreadPool to be queued and then executed.
     *
     * @param task      The Reaction task to be executed in the thread pool
     */
    void submit(std::unique_ptr<threading::ReactionTask>&& task) noexcept;

    /**
     * Log a message through NUClear's system.
     *
     * Logs a message through the system so the various log handlers can access it.
     * The arguments being logged should be able to be added into a stringstream.
     *
     * @tparam level     The level to log at (defaults to DEBUG)
     * @tparam Arguments The types of the arguments we are logging
     *
     * @param args The arguments we are logging
     */
    template <enum LogLevel level, typename... Arguments>
    void log(Arguments&&... args) {
        log(level, std::forward<Arguments>(args)...);
    }
    template <typename... Arguments>
    void log(const LogLevel& level, Arguments&&... args) {
        std::stringstream ss;
        log(level, ss, std::forward<Arguments>(args)...);
    }
    template <typename First, typename... Arguments>
    void log(const LogLevel& level, std::stringstream& ss, First&& first, Arguments&&... args) {
        ss << std::forward<First>(first) << " ";
        log(level, ss, std::forward<Arguments>(args)...);
    }
    template <typename Last>
    void log(const LogLevel& level, std::stringstream& ss, Last&& last) {
        ss << std::forward<Last>(last);
        log(level, ss);
    }
    void log(const LogLevel& level, std::stringstream& message);
    void log(const LogLevel& level, std::string message);

    /**
     * Emits data to the system and routes it to the other systems that use it.
     *
     * Emits at Local scope which creates tasks using the thread pool.
     *
     * @see NUClear::dsl::word::emit::Local for info about Local scope.
     *
     * @tparam T The type of the data that we are emitting
     *
     * @param data The data we are emitting
     */
    template <typename T>
    void emit(std::unique_ptr<T>&& data) {
        emit<dsl::word::emit::Local>(std::move(data));
    }
    template <typename T>
    void emit(std::unique_ptr<T>& data) {
        emit<dsl::word::emit::Local>(std::move(data));
    }
    template <template <typename> class First,
              template <typename>
              class... Remainder,
              typename T,
              typename... Arguments>
    void emit(std::unique_ptr<T>& data, Arguments&&... args) {
        emit<First, Remainder...>(std::move(data), std::forward<Arguments>(args)...);
    }

    template <template <typename> class First,
              template <typename>
              class... Remainder,
              typename T,
              typename... Arguments>
    void emit(std::unique_ptr<T>&& data, Arguments&&... args) {

        // Release our data from the pointer and wrap it in a shared_ptr
        emit_shared<First, Remainder...>(std::shared_ptr<T>(std::move(data)), std::forward<Arguments>(args)...);
    }

    template <template <typename> class First, template <typename> class... Remainder, typename... Arguments>
    void emit(Arguments&&... args) {

        emit_shared<First, Remainder...>(std::forward<Arguments>(args)...);
    }

    /**
     * Emits data to the system and routes it to the other systems that use it.
     *
     * This is for the special case of emitting a shared_ptr.
     * The types are Fused and the reaction is started.
     * If the Fusion fails, a static_assert fails.
     *
     * @note
     *  Emitting shared data can be helpful for forwarding data which has already been emitted and forwarding it on to
     *  external parties, without needing to copy it.
     *
     * @see NUClear::util::FunctionFusion
     *
     * @warning This shouldn't be used without a specific reason - usually forwarding data.
     *
     * @tparam First     The first handler to use for this emit
     * @tparam Remainder The remaining handlers to use for this emit
     * @tparam T         The type of the data that we are emitting
     * @tparam Arguments The additional arguments that will be provided to the handlers
     *
     * @param data The data we are emitting
     */
    template <template <typename> class First,
              template <typename>
              class... Remainder,
              typename T,
              typename... Arguments>
    void emit_shared(std::shared_ptr<T> data, Arguments&&... args) {

        using Functions      = std::tuple<First<T>, Remainder<T>...>;
        using ArgumentPack   = decltype(std::forward_as_tuple(*this, data, std::forward<Arguments>(args)...));
        using CallerArgs     = std::tuple<>;
        using FusionFunction = util::FunctionFusion<Functions, ArgumentPack, EmitCaller, CallerArgs, 2>;

        // Provide a check to make sure they are passing us the right stuff
        static_assert(
            FusionFunction::value,
            "There was an error with the arguments for the emit function, Check that your scope and arguments "
            "match what you are trying to do.");

        // Fuse our emit handlers and call the fused function
        FusionFunction::call(*this, data, std::forward<Arguments>(args)...);
    }

    template <template <typename> class First, template <typename> class... Remainder, typename... Arguments>
    void emit_shared(Arguments&&... args) {

        using Functions      = std::tuple<First<void>, Remainder<void>...>;
        using ArgumentPack   = decltype(std::forward_as_tuple(*this, std::forward<Arguments>(args)...));
        using CallerArgs     = std::tuple<>;
        using FusionFunction = util::FunctionFusion<Functions, ArgumentPack, EmitCaller, CallerArgs, 1>;

        // Provide a check to make sure they are passing us the right stuff
        static_assert(
            FusionFunction::value,
            "There was an error with the arguments for the emit function, Check that your scope and arguments "
            "match what you are trying to do.");

        // Fuse our emit handlers and call the fused function
        FusionFunction::call(*this, std::forward<Arguments>(args)...);
    }

    /// Our TaskScheduler that handles distributing task to the pool threads
    threading::scheduler::Scheduler scheduler;
    /// Our vector of Reactors, will get destructed when this vector is
    std::vector<std::unique_ptr<NUClear::Reactor>> reactors;
};

/**
 * This free floating log function can be called from anywhere and will use the singleton PowerPlant.
 *
 * @see NUClear::PowerPlant::log()
 *
 * @note The arguments being logged should be able to be added into a stringstream.
 *
 * @tparam level The LogLevel the message will be logged at. Defaults to DEBUG.
 * @tparam Arguments The types of the arguments to log.
 *
 * @param args The arguments to log.
 */
template <enum LogLevel level = NUClear::DEBUG, typename... Arguments>
void log(Arguments&&... args) {
    if (PowerPlant::powerplant != nullptr) {
        PowerPlant::powerplant->log<level>(std::forward<Arguments>(args)...);
    }
}
template <typename... Arguments>
void log(const LogLevel& level, Arguments&&... args) {
    if (PowerPlant::powerplant != nullptr) {
        PowerPlant::powerplant->log(level, std::forward<Arguments>(args)...);
    }
}

}  // namespace NUClear

#endif  // NUCLEAR_POWER_PLANT_HPP
