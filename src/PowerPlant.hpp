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

#ifndef NUCLEAR_POWERPLANT_HPP
#define NUCLEAR_POWERPLANT_HPP

#include <atomic>
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
#include "Configuration.hpp"
#include "LogLevel.hpp"
#include "id.hpp"
#include "message/LogMessage.hpp"
#include "threading/ReactionTask.hpp"
#include "threading/TaskScheduler.hpp"
#include "util/FunctionFusion.hpp"
#include "util/demangle.hpp"
#include "util/main_thread_id.hpp"
#include "util/unpack.hpp"

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
    // There can only be one powerplant, so this is it
    static PowerPlant* powerplant;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    /**
     * @brief
     *  Constructs a PowerPlant with the given configuration and provides access
     *  to argv for all reactors.
     *
     * @details Arguments passed to this function will be emitted as a CommandLineArguments message.
     *
     * @param config The PowerPlant's configuration
     * @param argc The number of command line arguments
     * @param argv The command line argument strings
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
     * @brief Starts up the PowerPlant's components in order and begins it running.
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
     * @brief Returns true if the PowerPlant is running or not intending to shut down soon. Returns false otherwise.
     */
    bool running() const;

    /**
     * @brief Installs a reactor of a particular type to the system.
     *
     * @details
     *  This function constructs a new Reactor of the template type.
     *  It passes through the specified LogLevel
     *  in the environment of that reactor so that it can be used to filter logs.
     *
     * @tparam T        The type of the reactor to build and install
     * @tparam Args     The types of the extra arguments to pass to the reactor constructor
     * @tparam level    The initial logging level for this reactor to use
     *
     * @param arg       Extra arguments to pass to the reactor constructor
     *
     * @return A reference to the installed reactor
     */
    template <typename T, typename... Args>
    T& install(Args&&... args);

    /**
     * @brief Generic submit function for submitting tasks to the thread pool.
     *
     * @param id        an id for ordering the task
     * @param priority  the priority of the task between 0 and 1000
     * @param group     the details of the execution group this task will run in
     * @param pool      the details of the thread pool this task will run from
     * @param immediate if this task should run immediately in the current thread
     * @param task      the wrapped function to be executed
     */
    void submit(const NUClear::id_t& id,
                const int& priority,
                const util::GroupDescriptor& group,
                const util::ThreadPoolDescriptor& pool,
                const bool& immediate,
                std::function<void()>&& task);

    /**
     * @brief Submits a new task to the ThreadPool to be queued and then executed.
     *
     * @param task The Reaction task to be executed in the thread pool
     * @param immediate if this task should run immediately in the current thread
     */
    void submit(std::unique_ptr<threading::ReactionTask>&& task, const bool& immediate = false) noexcept;

    /**
     * @brief Log a message through NUClear's system.
     *
     * @details
     *  Logs a message through the system so the various log handlers
     *  can access it. The arguments being logged should be able to
     *  be added into a stringstream.
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
     *  Emits at Local scope which creates tasks using the thread pool.
     *
     * @see NUClear::dsl::word::emit::Local for info about Local scope.
     *
     * @tparam T The type of the data that we are emitting
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
     *  This is for the special case of emitting a shared_ptr. The types are Fused and the reaction is started. If the
     *  Fusion fails, a static_assert fails.
     *
     * @note Emitting shared data can be helpful for forwarding data which has already been emitted and forwarding it on
     * to external parties, without needing to copy it.
     *
     * @see NUClear::util::FunctionFusion
     *
     * @warning This shouldn't be used without a specific reason - usually forwarding data.
     *
     * @tparam First        The first handler to use for this emit
     * @tparam Remainder    The remaining handlers to use for this emit
     * @tparam T            The type of the data that we are emitting
     * @tparam Arguments    The additional arguments that will be provided to the handlers
     *
     * @param data The data we are emitting
     */
    template <template <typename> class First,
              template <typename>
              class... Remainder,
              typename T,
              typename... Arguments>
    void emit_shared(std::shared_ptr<T> data, Arguments&&... args);

    template <template <typename> class First, template <typename> class... Remainder, typename... Arguments>
    void emit_shared(Arguments&&... args);

    template <template <typename> class First,
              template <typename>
              class... Remainder,
              typename T,
              typename... Arguments>
    void emit(std::unique_ptr<T>&& data, Arguments&&... args);

    template <template <typename> class First,
              template <typename>
              class... Remainder,
              typename T,
              typename... Arguments>
    void emit(std::unique_ptr<T>& data, Arguments&&... args);

    template <template <typename> class First, template <typename> class... Remainder, typename... Arguments>
    void emit(Arguments&&... args);

private:
    /// @brief A list of tasks that must be run when the powerplant starts up
    std::vector<std::function<void()>> tasks;
    /// @brief Our TaskScheduler that handles distributing task to the pool threads
    threading::TaskScheduler scheduler;
    /// @brief Our vector of Reactors, will get destructed when this vector is
    std::vector<std::unique_ptr<NUClear::Reactor>> reactors;
    /// @brief True if the powerplant is running
    std::atomic<bool> is_running{false};
};

/**
 * @brief This free floating log function can be called from anywhere and will use the singleton PowerPlant
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
    PowerPlant::log<level>(std::forward<Arguments>(args)...);
}

}  // namespace NUClear

// Include our Reactor.h first as the tight coupling between powerplant and reactor requires a specific include order
#include "Reactor.hpp"
#include "dsl/word/emit/Direct.hpp"
#include "dsl/word/emit/Initialise.hpp"
#include "dsl/word/emit/Local.hpp"
#include "message/CommandLineArguments.hpp"
#include "message/NetworkConfiguration.hpp"
#include "message/NetworkEvent.hpp"

// Include all of our implementation files (which use the previously included reactor.h)
#include "PowerPlant.ipp"

#endif  // NUCLEAR_POWERPLANT_HPP
