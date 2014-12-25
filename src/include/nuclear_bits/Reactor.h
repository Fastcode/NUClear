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

#include "nuclear_bits/util/demangle.h"

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
        
        ~Reactor() {}
        
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
        
        /// @copydoc dsl::word::With
        template <typename... TWiths>
        using With = dsl::word::With<TWiths...>;
        
        /// @copydoc dsl::word::Startup
        using Startup = dsl::word::Startup;
        
        /// @copydoc dsl::word::Shutdown
        using Shutdown = dsl::word::Shutdown;
        
        /// @copydoc dsl::word::Every
        template <int ticks, class period = std::chrono::milliseconds>
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
         * @param callback  The callback to execute when the trigger on this happens
         *
         * @return A ReactionHandle that controls if the created reaction runs or not
         */
        template <typename... TDSL, typename TFunc>
        void on(TFunc callback);
        
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
         * @param name      The name of this reaction to show in statistics
         * @param callback  The callback to execute when the trigger on this happens
         *
         * @return A ReactionHandle that controls if the created reaction runs or not
         */
        template <typename... TDSL, typename TFunc>
        void on(const std::string& name, TFunc&& callback);
        
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
        void emit(std::unique_ptr<TData>&& data);
        
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
        void log(TArgs... args);
    };
}

// We need to really make sure that PowerPlant is included as we use it in our ipp file
#include "nuclear_bits/PowerPlant.h"
#include "nuclear_bits/Reactor.ipp"

// Domain Specific Language
#include "nuclear_bits/dsl/word/Always.h"
#include "nuclear_bits/dsl/word/Trigger.h"
#include "nuclear_bits/dsl/word/With.h"
#include "nuclear_bits/dsl/word/Startup.h"
#include "nuclear_bits/dsl/word/Shutdown.h"
#include "nuclear_bits/dsl/word/Every.h"
#include "nuclear_bits/dsl/word/Single.h"
#include "nuclear_bits/dsl/word/Sync.h"

#endif

