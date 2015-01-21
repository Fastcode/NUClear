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

#ifndef NUCLEAR_REACTOR_H
#define NUCLEAR_REACTOR_H

#include <string>
#include <sstream>
#include <functional>
#include <vector>
#include <typeindex>
#include <chrono>
#include <atomic>

#include "nuclear_bits/util/Sequence.h"
#include "nuclear_bits/util/tuplify.h"

#include "nuclear_bits/dsl/Parse.h"

#include "nuclear_bits/Environment.h"
#include "nuclear_bits/threading/Reaction.h"
#include "nuclear_bits/threading/ReactionHandle.h"
#include "nuclear_bits/message/LogLevel.h"

namespace NUClear {
    
    // Domain specific language forward declaration
    namespace dsl {
        namespace word {
            
            struct Always;
            
            struct IO;
            
            struct UDP;
            
            template <typename>
            struct Trigger;
            
            template <typename...>
            struct With;
            
            struct Startup;
            
            struct Shutdown;
            
            template <int, typename>
            struct Every;
            
            template <typename>
            struct Per;
            
            struct Single;
            
            template <typename>
            struct Sync;
        }
    }
    
    /**
     * @brief Base class for any system that wants to react to events/data from the rest of the system.
     *
     * @details
     *  Provides functionality for binding callbacks to incoming data events. Callbacks are executed
     *  in a transparent, multithreaded manner. TODO needs to be expanded and updated
     *
     * @author Jake Woods
     * @author Trent Houliston
     * @version 1.1
     * @date 2-Apr-2013
     */
    class Reactor {
    public:
        friend class PowerPlant;
        
        Reactor(std::unique_ptr<Environment> environment)
          : environment(std::move(environment))
          , powerplant(this->environment->powerplant) {
        }
        
        ~Reactor() {
            
            // Unbind everything when we destroy the reactor
            for (auto& handle : reactionHandles) {
                handle.unbind();
            }
        }
        
    private:
        std::vector<threading::ReactionHandle> reactionHandles;
        
    protected:
        /// @brief Our environment
        std::unique_ptr<Environment> environment;
        
    public:
        /// @brief TODO
        PowerPlant& powerplant;
    protected:
        
        /***************************************************************************************************************
         * The types here are imported from other contexts so that when extending from the Reactor type in normal      *
         * usage there does not need to be any namespace declarations on the used types. This affords a simpler API    *
         * for the user.                                                                                               *
         **************************************************************************************************************/
        
        /// @copydoc dsl::word::Trigger
        template <typename... TTriggers>
        using Trigger = dsl::word::Trigger<TTriggers...>;
        
        /// @copydoc dsl::word::Always
        using Always = dsl::word::Always;
        
        /// @copydoc dsl::word::IO
        using IO = dsl::word::IO;
        
        /// @copydoc dsl::word::UDP
        using UDP = dsl::word::UDP;
        
        /// @copydoc dsl::word::With
        template <typename... TWiths>
        using With = dsl::word::With<TWiths...>;
        
        /// @copydoc dsl::word::Startup
        using Startup = dsl::word::Startup;
        
        /// @copydoc dsl::word::Shutdown
        using Shutdown = dsl::word::Shutdown;
        
        /// @copydoc dsl::word::Every
        template <int ticks = 0, class period = std::chrono::milliseconds>
        using Every = dsl::word::Every<ticks, period>;
        
        /// @copydoc dsl::word::Per
        template <class period>
        using Per = dsl::word::Per<period>;
        
        /// @copydoc dsl::word::Sync
        template <typename TSync>
        using Sync = dsl::word::Sync<TSync>;
        
        /// @copydoc dsl::word::Single
        using Single = dsl::word::Single;
        
        /// @brief This provides functions to modify how an on statement runs after it has been created
        using ReactionHandle = threading::ReactionHandle;
        
        template <typename DSL, typename... TArgs>
        struct Binder {
        private:
            Reactor& reactor;
            std::tuple<TArgs...> args;
            
            // On a reaction handle add it to our list
            void addReactionHandle(ReactionHandle& r) {
                reactor.reactionHandles.push_back(r);
            }
            
            // Do nothing if it's not a reaction handle
            void addReactionHandle(...) {}
            
            template <int i = 0, typename... Tp>
            util::Meta::EnableIf<std::integral_constant<bool, i == sizeof...(Tp)>> addReactionHandles(std::tuple<Tp...>&) {}
            
            template <int i = 0, typename... Tp>
            util::Meta::EnableIf<std::integral_constant<bool, i < sizeof...(Tp)>> addReactionHandles(std::tuple<Tp...>& t) {
                addReactionHandle(std::get<i>(t));
            }
            
            template<std::size_t I = 0, typename FuncT, typename... Tp>
            inline typename std::enable_if<I < sizeof...(Tp), void>::type
            for_each(std::tuple<Tp...>& t, FuncT f)
            {
                f(std::get<I>(t));
                for_each<I + 1, FuncT, Tp...>(t, f);
            }
            
            
            template <typename TFunc, int... Index>
            auto then(const std::string& label, TFunc&& callback, const util::Sequence<Index...>&)
            -> decltype(util::detuplify(DSL::bind(reactor, label, std::forward<TFunc>(callback), std::get<Index>(args)...))) {
                
                // Get our tuple from binding our reaction
                auto tuple = DSL::bind(reactor, label, std::forward<TFunc>(callback), std::get<Index>(args)...);
                
                // Get all reaction handles from the tuple and put them into our global list so we can debind them on destruction
                addReactionHandles(tuple);
                
                // Return the arguments to the user (if there is only 1 we unwrap it for them since this is the most common case)
                return util::detuplify(std::move(tuple));
                
                // Put all of the handles into our global list so we can debind them on destruction
                //                reactor.reactionHandles.insert(std::end(reactor.reactionHandles), std::begin(handles), std::end(handles));
                
                //                return handles;
            }
            
        public:
            Binder(Reactor& r, TArgs&&... args)
            : reactor(r)
            , args(args...) {}
            
            template <typename TFunc>
            auto then(const std::string& label, TFunc&& callback)
            -> decltype(then(label, std::forward<TFunc>(callback), util::GenerateSequence<sizeof...(TArgs)>())) {
                return then(label, std::forward<TFunc>(callback), util::GenerateSequence<sizeof...(TArgs)>());
            }
            
            template <typename TFunc>
            auto then(TFunc&& callback)
            -> decltype(then("", std::forward<TFunc>(callback))) {
                return then("", std::forward<TFunc>(callback));
            }
        };
        
        // FUNCTIONS

        /**
         * @brief The on function is the method used to create a reaction in the NUClear system.
         *
         * @details
         *  This function is used to create a Reaction in the system. By providing the correct
         *  template parameters, this function can modify how and when this reaction runs.
         *
         * @tparam TDSL     The NUClear domain specific language information
         * @tparam TFunc    The type of the function passed in
         *
         * @param args      The arguments that will be passed to each of the binding DSL words in order
         *
         * @return A Binder object that can be used to bind callbacks to this DSL statement
         */
        template <typename... TDSL, typename... TArgs>
        Binder<dsl::Parse<TDSL...>, TArgs...> on(TArgs&&... args) {
            
            // There must be some parameters
            static_assert(sizeof...(TDSL) > 0, "You must have at least one parameter in an on");
            
            return Binder<dsl::Parse<TDSL...>, TArgs...>(*this, std::forward<TArgs>(args)...);
        }
        
        /**
         * @brief Emits data into the system so that other reactors can use it.
         *
         * @details
         *  This function emits data to the rest of the system so that it can be used.
         *  This results in it being the new data used when a with is used, and triggering
         *  any reaction that is set to be triggered on this data type.
         *
         *
         * @tparam THandlers    The handlers for this emit (e.g. LOCAL, NETWORK etc)
         * @tparam TData        The type of the data we are emitting
         *
         * @param data The data to emit
         */
        template <typename... THandlers, typename TData>
        void emit(std::unique_ptr<TData>&& data) {
            powerplant.emit<THandlers...>(std::forward<std::unique_ptr<TData>>(data));
        }
        
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
        template <enum LogLevel level = DEBUG, typename... TArgs>
        void log(TArgs... args) {
            
            // If the log is above or equal to our log level
            if (level >= environment->logLevel) {
                powerplant.log<level, TArgs...>(std::forward<TArgs>(args)...);
            }
        }
    };
}

// Domain Specific Language
#include "nuclear_bits/dsl/word/Always.h"
#include "nuclear_bits/dsl/word/IO.h"
#include "nuclear_bits/dsl/word/UDP.h"
#include "nuclear_bits/dsl/word/Trigger.h"
#include "nuclear_bits/dsl/word/With.h"
#include "nuclear_bits/dsl/word/Startup.h"
#include "nuclear_bits/dsl/word/Shutdown.h"
#include "nuclear_bits/dsl/word/Every.h"
#include "nuclear_bits/dsl/word/Single.h"
#include "nuclear_bits/dsl/word/Sync.h"

#endif

