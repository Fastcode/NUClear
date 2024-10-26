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

#ifndef NUCLEAR_REACTOR_HPP
#define NUCLEAR_REACTOR_HPP

#include <chrono>
#include <functional>
#include <regex>
#include <string>
#include <typeindex>
#include <vector>

#include "Environment.hpp"
#include "LogLevel.hpp"
#include "dsl/Parse.hpp"
#include "threading/Reaction.hpp"
#include "threading/ReactionHandle.hpp"
#include "threading/ReactionIdentifiers.hpp"
#include "util/CallbackGenerator.hpp"
#include "util/Sequence.hpp"
#include "util/demangle.hpp"
#include "util/tuplify.hpp"

namespace NUClear {

// Domain specific language forward declaration
namespace dsl {
    namespace word {

        struct Always;

        struct Once;

        struct Priority;

        template <typename>
        struct Idle;

        struct IO;

        struct UDP;

        struct TCP;

        template <typename...>
        struct Optional;

        template <size_t, typename...>
        struct Last;

        struct MainThread;

        template <typename T>
        struct TaskScope;

        template <typename>
        struct Network;

        struct NetworkSource;

        struct Inline;

        template <typename>
        struct Trigger;

        template <typename>
        struct With;

        struct Startup;

        struct Shutdown;

        template <int, typename>
        struct Every;

        template <typename, int, typename>
        struct Watchdog;

        template <typename>
        struct Per;

        struct Single;

        template <int>
        struct Buffer;

        template <typename>
        struct Sync;

        template <typename>
        struct Pool;

        template <typename>
        struct Group;

        namespace emit {
            template <typename T>
            struct Local;
            template <typename T>
            struct Inline;
            template <typename T>
            struct Delay;
            template <typename T>
            struct Initialise;
            template <typename T>
            struct Network;
            template <typename T>
            struct UDP;
            template <typename T>
            struct Watchdog;
            template <typename WatchdogGroup, typename RuntimeType>
            struct WatchdogServicer;
            template <typename WatchdogGroup, typename RuntimeType>
            WatchdogServicer<WatchdogGroup, RuntimeType> ServiceWatchdog(RuntimeType&& data);
            template <typename WatchdogGroup>
            WatchdogServicer<WatchdogGroup, void> ServiceWatchdog();
        }  // namespace emit
    }  // namespace word
}  // namespace dsl

/**
 * Base class for any system that wants to react to events/data from the rest of the system.
 *
 * Provides functionality for binding callbacks to incoming data events.
 * Callbacks are executed in a transparent, multithreaded manner.
 *
 * TODO needs to be expanded and updated.
 */
class Reactor {
public:
    friend class PowerPlant;

    explicit Reactor(std::unique_ptr<Environment> environment)
        : powerplant(environment->powerplant), reactor_name(environment->reactor_name) {}

    // Copying or moving a Reactor is almost certainly a mistake
    Reactor(const Reactor& /*other*/)              = delete;
    Reactor(Reactor&& /*other*/) noexcept          = delete;
    Reactor& operator=(const Reactor& /*rhs*/)     = delete;
    Reactor& operator=(Reactor&& /*rhs*/) noexcept = delete;

    virtual ~Reactor() {

        // Unbind everything when we destroy the reactor
        for (auto& handle : reaction_handles) {
            handle.unbind();
        }
    }

private:
    std::vector<threading::ReactionHandle> reaction_handles;

public:
    /// The powerplant that this reactor is running in
    PowerPlant& powerplant;

    /// The demangled string name of this reactor
    const std::string reactor_name;

protected:
    /// The level that this reactor logs at
    LogLevel log_level{LogLevel::INFO};

    /***************************************************************************************************************
     * The types here are imported from other contexts so that when extending from the Reactor type in normal      *
     * usage there does not need to be any namespace declarations on the used types.                               *
     * This affords a simpler API for the user.                                                                    *
     **************************************************************************************************************/

    /// @copydoc dsl::word::Trigger
    template <typename T>
    using Trigger = dsl::word::Trigger<T>;

    /// @copydoc dsl::word::Priority
    using Priority = dsl::word::Priority;

    /// @copydoc dsl::word::Always
    using Always = dsl::word::Always;

    /// @copydoc dsl::word::Once
    using Once = dsl::word::Once;

    /// @copydoc dsl::word::Idle
    template <typename T = void>
    using Idle = dsl::word::Idle<T>;

    /// @copydoc dsl::word::IO
    using IO = dsl::word::IO;

    /// @copydoc dsl::word::UDP
    using UDP = dsl::word::UDP;

    /// @copydoc dsl::word::TCP
    using TCP = dsl::word::TCP;

    /// @copydoc dsl::word::With
    template <typename T>
    using With = dsl::word::With<T>;

    /// @copydoc dsl::word::Optional
    template <typename... DSL>
    using Optional = dsl::word::Optional<DSL...>;

    /// @copydoc dsl::word::Last
    template <size_t len, typename... DSL>
    using Last = dsl::word::Last<len, DSL...>;

    /// @copydoc dsl::word::MainThread
    using MainThread = dsl::word::MainThread;

    /// @copydoc dsl::word::TaskScope
    template <typename T>
    using TaskScope = dsl::word::TaskScope<T>;

    /// @copydoc dsl::word::Startup
    using Startup = dsl::word::Startup;

    /// @copydoc dsl::word::Network
    template <typename T>
    using Network = dsl::word::Network<T>;

    /// @copydoc dsl::word::Network
    using NetworkSource = dsl::word::NetworkSource;

    /// @copydoc dsl::word::Inline
    using Inline = dsl::word::Inline;

    /// @copydoc dsl::word::Shutdown
    using Shutdown = dsl::word::Shutdown;

    /// @copydoc dsl::word::Pool
    template <typename T = dsl::word::pool::Default>
    using Pool = dsl::word::Pool<T>;

    /// @copydoc dsl::word::Group
    template <typename T>
    using Group = dsl::word::Group<T>;

    /// @copydoc dsl::word::Every
    template <int ticks = 0, class period = std::chrono::milliseconds>
    using Every = dsl::word::Every<ticks, period>;

    /// @copydoc dsl::word::Watchdog
    template <typename TWatchdog, int ticks, class period = std::chrono::milliseconds>
    using Watchdog = dsl::word::Watchdog<TWatchdog, ticks, period>;

    /// @copydoc dsl::word::emit::ServiceWatchdog
    template <typename WatchdogGroup, typename... Arguments>
    auto ServiceWatchdog(Arguments&&... args)
        // THIS IS VERY IMPORTANT, the return type must be dependent on the function call
        // otherwise it won't check it's valid in SFINAE
        -> decltype(dsl::word::emit::ServiceWatchdog<WatchdogGroup>(std::forward<Arguments>(args)...)) {
        return dsl::word::emit::ServiceWatchdog<WatchdogGroup>(std::forward<Arguments>(args)...);
    }

    /// @copydoc dsl::word::Per
    template <class period>
    using Per = dsl::word::Per<period>;

    /// @copydoc dsl::word::Sync
    template <typename SyncGroup>
    using Sync = dsl::word::Sync<SyncGroup>;

    /// @copydoc dsl::word::Single
    using Single = dsl::word::Single;

    /// @copydoc dsl::word::Buffer
    template <int N>
    using Buffer = dsl::word::Buffer<N>;

    struct Scope {
        /// @copydoc dsl::word::emit::Local
        template <typename T>
        using LOCAL = dsl::word::emit::Local<T>;

        /// @copydoc dsl::word::emit::Inline
        template <typename T>
        using INLINE = dsl::word::emit::Inline<T>;

        /// @copydoc dsl::word::emit::Delay
        template <typename T>
        using DELAY = dsl::word::emit::Delay<T>;

        /// @copydoc dsl::word::emit::Initialise
        template <typename T>
        using INITIALIZE = dsl::word::emit::Initialise<T>;

        /// @copydoc dsl::word::emit::Network
        template <typename T>
        using NETWORK = dsl::word::emit::Network<T>;

        /// @copydoc dsl::word::emit::UDP
        template <typename T>
        using UDP = dsl::word::emit::UDP<T>;

        /// @copydoc dsl::word::emit::WATCHDOG
        template <typename T>
        using WATCHDOG = dsl::word::emit::Watchdog<T>;
    };

    /// This provides functions to modify how an on statement runs after it has been created
    using ReactionHandle = threading::ReactionHandle;

public:
    template <typename DSL, typename... Arguments>
    struct Binder {
    private:
        Reactor& reactor;
        std::tuple<Arguments...> args;

        template <typename Function, int... Index>
        auto then(const std::string& label, Function&& callback, const util::Sequence<Index...>& /*s*/) {

            // Regex replace NUClear::dsl::Parse with NUClear::Reactor::on so that it reads more like what is expected
            const std::string dsl = std::regex_replace(util::demangle(typeid(DSL).name()),
                                                       std::regex("NUClear::dsl::Parse<"),
                                                       "NUClear::Reactor::on<");

            // Generate the identifier
            threading::ReactionIdentifiers identifiers{label,
                                                       reactor.reactor_name,
                                                       dsl,
                                                       util::demangle(typeid(Function).name())};

            // Generate the reaction
            auto reaction = std::make_shared<threading::Reaction>(
                reactor,
                std::move(identifiers),
                util::CallbackGenerator<DSL, Function>(std::forward<Function>(callback)));

            // Get our tuple from binding our reaction
            auto tuple = DSL::bind(reaction, std::get<Index>(args)...);

            auto handle = threading::ReactionHandle(reaction);
            reactor.reaction_handles.push_back(handle);

            // Return the arguments to the user (if there is only 1 we unwrap it for them since this is the most common
            // case)
            return util::detuplify(std::tuple_cat(std::make_tuple(handle), tuple));
        }

    public:
        Binder(Reactor& r, Arguments&&... args) : reactor(r), args(args...) {}

        template <typename Label, typename Function>
        auto then(Label&& label, Function&& callback) {
            return then(std::forward<Label>(label),
                        std::forward<Function>(callback),
                        util::GenerateSequence<0, sizeof...(Arguments)>());
        }

        template <typename Function>
        auto then(Function&& callback) {
            return then("", std::forward<Function>(callback));
        }
    };

    // FUNCTIONS

    /**
     * The on function is the method used to create a reaction in the NUClear system.
     *
     * This function is used to create a Reaction in the system.
     * By providing the correct template parameters, this function can modify how and when this reaction runs.
     *
     * @tparam DSL          The NUClear domain specific language information
     * @tparam Arguments    The types of the arguments passed into the function
     *
     * @param args      The arguments that will be passed to each of the binding DSL words in order
     *
     * @return A Binder object that can be used to bind callbacks to this DSL statement
     */
    template <typename... DSL, typename... Arguments>
    Binder<dsl::Parse<DSL...>, Arguments...> on(Arguments&&... args) {

        // There must be some parameters
        static_assert(sizeof...(DSL) > 0, "You must have at least one parameter in an on");

        return Binder<dsl::Parse<DSL...>, Arguments...>(*this, std::forward<Arguments>(args)...);
    }

    /**
     * Emits data into the system so that other reactors can use it.
     *
     * This function emits data to the rest of the system so that it can be used.
     * This results in it being the new data used when a with is used, and triggering any reaction that is set to be
     * triggered on this data type.
     *
     * @tparam Handlers The handlers for this emit (e.g. LOCAL, NETWORK etc)
     * @tparam T        The type of the data we are emitting, for some handlers (e.g. WATCHDOG) this is optional
     *
     * @param data The data to emit, for some handlers (e.g. WATCHDOG) this is optional
     */
    template <template <typename> class... Handlers, typename T, typename... Arguments>
    void emit(std::unique_ptr<T>&& data, Arguments&&... args) {
        powerplant.emit<Handlers...>(std::move(data), std::forward<Arguments>(args)...);
    }
    template <template <typename> class... Handlers, typename T, typename... Arguments>
    void emit(std::unique_ptr<T>& data, Arguments&&... args) {
        powerplant.emit<Handlers...>(std::move(data), std::forward<Arguments>(args)...);
    }
    template <template <typename> class... Handlers, typename... Arguments>
    void emit(Arguments&&... args) {
        powerplant.emit<Handlers...>(std::forward<Arguments>(args)...);
    }

    /**
     * Log a message through NUClear's system.
     *
     * Logs a message through the system so the various log handlers can access it.
     *
     * @tparam level The level to log at (defaults to DEBUG)
     * @tparam Arguments The types of the arguments we are logging
     *
     * @param args The arguments we are logging
     */
    template <enum LogLevel level = DEBUG, typename... Arguments>
    void log(Arguments&&... args) const {

        // If the log is above or equal to our log level
        powerplant.log<level>(std::forward<Arguments>(args)...);
    }

    /**
     * Log a message through NUClear's system.
     *
     * Logs a message through the system so the various log handlers can access it.
     *
     * @tparam Arguments The types of the arguments we are logging
     *
     * @param level The level to log at
     * @param args The arguments we are logging
     */
    template <typename... Arguments>
    void log(const LogLevel& level, Arguments&&... args) const {

        // If the log is above or equal to our log level
        powerplant.log(level, std::forward<Arguments>(args)...);
    }
};

}  // namespace NUClear

// Domain Specific Language
#include "dsl/word/Always.hpp"
#include "dsl/word/Buffer.hpp"
#include "dsl/word/Every.hpp"
#include "dsl/word/Group.hpp"
#include "dsl/word/IO.hpp"
#include "dsl/word/Idle.hpp"
#include "dsl/word/Inline.hpp"
#include "dsl/word/Last.hpp"
#include "dsl/word/MainThread.hpp"
#include "dsl/word/Network.hpp"
#include "dsl/word/Once.hpp"
#include "dsl/word/Optional.hpp"
#include "dsl/word/Pool.hpp"
#include "dsl/word/Priority.hpp"
#include "dsl/word/Shutdown.hpp"
#include "dsl/word/Single.hpp"
#include "dsl/word/Startup.hpp"
#include "dsl/word/Sync.hpp"
#include "dsl/word/TCP.hpp"
#include "dsl/word/TaskScope.hpp"
#include "dsl/word/Trigger.hpp"
#include "dsl/word/UDP.hpp"
#include "dsl/word/Watchdog.hpp"
#include "dsl/word/With.hpp"
#include "dsl/word/emit/Delay.hpp"
#include "dsl/word/emit/Initialise.hpp"
#include "dsl/word/emit/Inline.hpp"
#include "dsl/word/emit/Local.hpp"
#include "dsl/word/emit/Network.hpp"
#include "dsl/word/emit/UDP.hpp"
#include "dsl/word/emit/Watchdog.hpp"

#endif  // NUCLEAR_REACTOR_HPP
