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

#ifndef NUCLEAR_REACTOR_H
#define NUCLEAR_REACTOR_H

#include <string>
#include <functional>
#include <vector>
#include <typeindex>
#include <chrono>
#include <atomic>
#include "NUClear/Internal/Reaction.h"
#include "NUClear/Internal/CommandTypes/CommandTypes.h"
#include "NUClear/Internal/Magic/unpack.h"
#include "NUClear/Internal/Magic/apply.h"
#include "NUClear/Internal/Magic/TypeMap.h"
#include "NUClear/Internal/Magic/buildVector.h"

namespace NUClear {
    
    class PowerPlant;

    /**
     * @brief Base class for any system that wants to react to events/data from the rest of the system.
     * @details 
     *  Provides functionality for binding callbacks to incoming data events. Callbacks are executed
     *  in a transparent, multithreaded manner.
     * @author Jake Woods
     * @author Trent Houliston
     * @version 1.1
     * @date 2-Apr-2013
     */
    class Reactor {
        public:
            friend class PowerPlant;

            Reactor(PowerPlant& powerPlant);
            ~Reactor();
 
        protected:
            PowerPlant& powerPlant;
        
            // IMPORTED TYPES TO ALLOW LOCAL USAGE
        
            using time_t = std::chrono::time_point<std::chrono::steady_clock>;
        
            template <typename... TTriggers>
            using Trigger = Internal::CommandTypes::Trigger<TTriggers...>;
        
            template <typename... TWiths>
            using With = Internal::CommandTypes::With<TWiths...>;
        
            using Scope = Internal::CommandTypes::Scope;
        
            template <typename... TOptions>
            using Options = Internal::CommandTypes::Options<TOptions...>;
        
            using Initialize = Internal::CommandTypes::Initialize;
        
            using Shutdown = Internal::CommandTypes::Shutdown;
        
            template <int ticks, class period = std::chrono::milliseconds>
            using Every = Internal::CommandTypes::Every<ticks, period>;
        
            template <int num, class TData>
            using Last = Internal::CommandTypes::Last<num, TData>;
        
            template <enum EPriority P>
            using Priority = Internal::CommandTypes::Priority<P>;
        
            template <typename TSync>
            using Sync = Internal::CommandTypes::Sync<TSync>;
        
            template <typename TData, int index = 0>
            using Linked = Internal::CommandTypes::Linked<TData, index>;
        
            template <typename TData>
            using Network = Internal::CommandTypes::Network<TData>;

            using Single = Internal::CommandTypes::Single;

            // FUNCTIONS
        
            template <typename TTrigger, typename TFunc>
            void on(TFunc callback); 

            template <typename TTrigger, typename TWithOrOption, typename TFunc>
            void on(TFunc callback); 

            template <typename TTrigger, typename TWith, typename TOption, typename TFunc>
            void on(TFunc callback);
        
            template <enum Internal::CommandTypes::Scope... TScopes, typename TData>
            void emit(TData* data);
        private:
        
            template <typename TKey>
            using CallbackCache = Internal::Magic::TypeList<Reactor, TKey, Internal::Reaction>;
        
            /**
             * @brief Base template instantitation that gets specialized. 
             * @details 
             *  This should never be instantiated and will throw a giant compile error if it somehow is. 
             *  The template parameters are left unnamed to reflect the fact that they are simply placeholders.
             */
            template <typename, typename, typename, typename>
            struct OnImpl {};

            /**
             * @brief Standard Trigger<...>, With<...> specialization of OnImpl.
             * @details 
             *  This class essentially acts as a polymorphic lambda function in that it allows us
             *  to select what specialization of OnImpl we want based on the compile-time template arguments.
             *  In this case this specialization matches OnImpl<Trigger<...>, With<...>>(callback) such that
             *  we can get the individual "trigger" and "with" lists. Essentially, think of this as a function 
             *  rather then a class as that's what it would be if C++ allowed partial template specialization of
             *  functions.
             * @tparam TTriggers the list of events/data that trigger this reaction
             * @tparam TWiths the list of events/data that is required for this reaction but does not trigger the reaction.
             * @tparam TFunc the callback type, should be automatically deduced
             */
            template <typename... TTriggers, typename... TWiths, typename... TOptions, typename TFunc>
            struct OnImpl<Trigger<TTriggers...>, With<TWiths...>, Options<TOptions...>, TFunc> {
                Reactor* context;
                OnImpl(Reactor* context); 
                void operator()(TFunc callback);            
            };
        
            /**
             * @brief This function populates the passed ReactionOptions object will all of the data held in the options
             * @tparam the options which to apply to this options object
             * @param the options to populate
             */
            template <typename... TOption>
            void buildOptions(Internal::Reaction::Options& options);
        
            /**
             * @brief This case of build options is used when the Single option is specified
             * @param options the options object we are building
             * @param placeholder which is used to specialize this method
             */
            void buildOptionsImpl(Internal::Reaction::Options& options, Single* /*placeholder*/);
        
            /**
             * @brief This case of build options is used to add the Sync option
             * @tparam TSync the sync type we are synchronizing on
             * @param placeholder which is used to specialize this method
             */
            template <typename TSync>
            void buildOptionsImpl(Internal::Reaction::Options& options, Sync<TSync>* /*placeholder*/);
        
            /**
             * @brief This case of build options is used to add the Priority option
             * @tparam TSync the sync type we are synchronizing on
             * @param placeholder which is used to specialize this method
             */
            template <enum EPriority P>
            void buildOptionsImpl(Internal::Reaction::Options& options, Priority<P>* /*placeholder*/);
        
            template <typename TType>
            struct Exists;
        
            /**
             * @brief Builds a callback based on if it needs a second pass at runtime.
             *
             * @details
             *  If this is run with true, then when the callback is executed, it will have it's second pass run to
             *  get additional data.
             *
             * @tparam secondPass if a second pass over the paramters needs to be done when the callback is executed.
             *
             */
            template <bool secondPass>
            struct buildCallback;
        
            /**
             * @brief Builds a callback wrapper function for a given callback. 
             * @details 
             *  All callbacks are wrapped in a void() lambda that knows how to get the correct arguments
             *  when called. This is done so all callbacks can be stored and called in a uniform way.
             * @tparam TFunc the callback type
             * @tparam TTriggersAndWiths the list of all triggers and required data for this reaction
             * @param callback the callback function
             * @returns The wrapped callback
             */
            template <typename TFunc, typename... TTriggersAndWiths>
            Internal::Reaction* buildReaction(TFunc callback, Internal::Reaction::Options& options);
        
            /**
             * @brief Adds a single data -> callback mapping for a single type.
             * @tparam TTrigger the event/data type to add the callback for
             * @param callback the callback to add
             */
            template <typename TTrigger, typename... TTriggers>
            void bindTriggers(Internal::Reaction* callback);
        
            template <typename TType>
            struct TriggerType;
    };
}

#include "NUClear/PowerPlant.h"
#include "NUClear/Reactor.ipp"
#endif

