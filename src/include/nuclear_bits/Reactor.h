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
#include "nuclear_bits/threading/Reaction.h"
#include "nuclear_bits/threading/ReactionHandle.h"
#include "nuclear_bits/dsl/dsl.h"
#include "nuclear_bits/metaprogramming/unpack.h"
#include "nuclear_bits/metaprogramming/apply.h"
#include "nuclear_bits/metaprogramming/TypeMap.h"
#include "nuclear_bits/metaprogramming/MetaProgramming.h"
#include "nuclear_bits/ForwardDeclarations.h"

namespace NUClear {
    
    // Import our meta programming utility
    using namespace metaprogramming;
    
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
        
        Reactor(std::unique_ptr<Environment> environment);
        ~Reactor();
        
    protected:
        /// @brief Our environment
        std::unique_ptr<Environment> environment;
        
        /// @brief TODO
        PowerPlant& powerplant;
        
        /***************************************************************************************************************
         * The types here are imported from other contexts so that when extending from the Reactor type in normal      *
         * usage there does not need to be any namespace declarations on the used types. This affords a simpler API    *
         * for the user.                                                                                               *
         **************************************************************************************************************/
        
        /// @brief The Time units used by the NUClear system
        using time_t = clock::time_point;
        
        /// @copydoc dsl::Trigger
        template <typename... TTriggers>
        using Trigger = dsl::Trigger<TTriggers...>;
        
        /// @copydoc dsl::With
        template <typename... TWiths>
        using With = dsl::With<TWiths...>;
        
        /// @copydoc dsl::Scope
        using Scope = dsl::Scope;
        
        /// @copydoc dsl::Options
        template <typename... TOptions>
        using Options = dsl::Options<TOptions...>;
        
        /// @copydoc dsl::Startup
        using Startup = dsl::Startup;
        
        /// @copydoc dsl::Shutdown
        using Shutdown = dsl::Shutdown;
        
        /// @copydoc dsl::Every
        template <int ticks, class period = std::chrono::milliseconds>
        using Every = dsl::Every<ticks, period>;
        
        /// @copydoc dsl::Per
        template <class period>
        using Per = dsl::Per<period>;
        
        /// @copydoc dsl::Raw
        template <typename TData>
        using Raw = dsl::Raw<TData>;
        
        /// @copydoc dsl::Last
        template <int num, class TData>
        using Last = dsl::Last<num, TData>;
        
        /// @brief The type of data that is returned by Last<num, TData>
        template <class TData>
        using LastList = std::vector<std::shared_ptr<const TData>>;
        
        /// @copydoc dsl::CommandLineArguments
        using CommandLineArguments = dsl::CommandLineArguments;
        
        /// @copydoc dsl::Priority
        template <enum EPriority P>
        using Priority = dsl::Priority<P>;
        
        /// @copydoc dsl::Network
        template <typename TData>
        using Network = dsl::Network<TData>;
        
        /// @copydoc dsl::Sync
        template <typename TSync>
        using Sync = dsl::Sync<TSync>;
        
        /// @copydoc dsl::Single
        using Single = dsl::Single;
        
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
         *
         * @tparam TParams  The parameters of the Every class
         * @tparam TFunc    The type of the function passed in
         *
         * @param callback  The callback to execute when the trigger on this happens
         *
         * @return A ReactionHandle that controls if the created reaction runs or not
         */
        template <typename... TParams, typename TFunc>
        Reactor::ReactionHandle on(TFunc callback);
        
        /**
         * @brief The on function is the method used to create a reaction in the NUClear system.
         *
         * @details
         *  This function is used to create a Reaction in the system. By providing the correct
         *  template parameters, this function can modify how and when this reaction runs.
         *
         *
         * @tparam TParams  The parameters of the Every class
         * @tparam TFunc    The type of the function passed in
         *
         * @param name      The name of this reaction to show in statistics
         * @param callback  The callback to execute when the trigger on this happens
         *
         * @return A ReactionHandle that controls if the created reaction runs or not
         */
        template <typename... TParams, typename TFunc>
        Reactor::ReactionHandle on(const std::string& name, TFunc callback);
        
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
        
    private:
        /// @brief The static cache where we link our callbacks to the ReactorMaster
        template <typename TKey>
        using CallbackCache = metaprogramming::TypeList<Reactor, TKey, std::unique_ptr<threading::Reaction>>;
        
        /**
         * @brief Base template instantitation that gets specialized.
         *
         * @details
         *  This should never be instantiated and will throw a giant compile error if it somehow is.
         *  The template parameters are left unnamed to reflect the fact that they are simply placeholders.
         */
        template <typename, typename...>
        struct On;
        
        /**
         * @brief This type is used to build the final On statement that will be called.
         *
         * @details
         *  This type self extends to rearrange the arguments in the On until they are useable by the system.
         */
        template <typename, typename...>
        struct OnBuilder;
        
        /**
         * @brief This metafunction will check that the function provided to the system is compatible with
         *  the 
         *
         * @tparam TFunc    the function object that we are checking.
         * @tparam TTuple   the expected arguments provided by the function.
         * @tparam Stage    what stage of the function checking we are up to.
         *
         */
        template <typename TFunc, typename TTuple, int Stage = 0>
        struct CheckFunctionSignature;
        
        /**
         * @brief Standard Trigger<...>, With<...> specialization of On.
         *
         * @details
         *  This class essentially acts as a polymorphic lambda function in that it allows us
         *  to select what specialization of On we want based on the compile-time template arguments.
         *  In this case this specialization matches OnImpl<Trigger<...>, With<...>>(callback) such that
         *  we can get the individual "trigger" and "with" lists. Essentially, think of this as a function
         *  rather then a class as that's what it would be if C++ allowed partial template specialization of
         *  functions.
         *
         * @tparam TTriggers    the list of events/data that trigger this reaction
         * @tparam TWiths       the list of events/data that is required for this reaction but does not trigger the
         *                      reaction.
         * @tparam TOptions     the options that the callback is executed with
         * @tparam TFunc        the callback type, should be automatically deduced
         */
        template <typename TFunc, typename... TTriggers, typename... TWiths, typename... TOptions, typename... TFuncArgs>
        struct On<TFunc, Trigger<TTriggers...>, With<TWiths...>, Options<TOptions...>, std::tuple<TFuncArgs...>> {
            
            static ReactionHandle on(Reactor& context, std::string name, TFunc callback);
        };
        
        /**
         * @brief This function populates the passed ReactionOptions object will all of the data held in the options.
         *
         * @tparam TOption the options which to apply to this options object
         *
         * @param options the options to populate
         */
        template <typename... TOption>
        void buildOptions(threading::ReactionOptions& options);
        
        /*
         * NOTE THAT THE FUNCTIONS BELOW USE EMPTY POINTERS TO CHOOSE WHICH FUNCTION TO EXECUTE
         * (as you can't partially specialize functions)
         */
        
        /**
         * @brief This case of build options is used when the Single option is specified.
         *
         * @param options the options object we are building
         */
        void buildOptionsImpl(threading::ReactionOptions& options, Single*);
        
        /**
         * @brief This case of build options is used to add the Sync option.
         *
         * @tparam TSync the sync type we are synchronizing on
         *
         * @param placeholder which is used to specialize this method
         */
        template <typename TSync>
        void buildOptionsImpl(threading::ReactionOptions& options, Sync<TSync>*);
        
        /**
         * @brief This case of build options is used to add the Priority option.
         *
         * @tparam P the sync type we are synchronizing on
         *
         * @param placeholder which is used to specialize this method
         */
        template <enum EPriority P>
        void buildOptionsImpl(threading::ReactionOptions& options, Priority<P>*);
        
        /**
         * @brief This extension point will execute when a paticular type exists within an on statement.
         *
         * @details
         *  The function on the Exists will execute when a type is in a Trigger or With. This allows
         *  functions to setup ready for the ons or triggers that will require this kind of data.
         *
         * @tparam TData The datatype that exists in a statement
         */
        template <typename TData>
        struct Exists;
        
        /**
         * @brief Builds a callback wrapper function for a given callback. TODO this needs updating
         *
         * @details
         *  All callbacks are wrapped in a void() lambda that knows how to get the correct arguments
         *  when called. This is done so all callbacks can be stored and called in a uniform way.
         *
         * @tparam TFunc the callback type
         * @tparam TTriggersAndWiths the list of all triggers and required data for this reaction
         *
         * @param callback the callback function
         *
         * @return The wrapped callback
         */
        template <typename TFunc, typename... TTriggersAndWiths>
        std::unique_ptr<threading::Reaction> buildReaction(std::string name, TFunc callback, threading::ReactionOptions& options);
        
        /**
         * @brief Adds a single data -> callback mapping for a single type.
         *
         * @tparam TTrigger the event/data type to add the callback for
         *
         * @param callback the callback to add
         *
         * @return A handler which allows us to modify our Reaction at runtime
         */
        template <typename TTrigger, typename... TTriggers>
        ReactionHandle bindTriggers(std::unique_ptr<threading::Reaction>&& callback);
        
        /**
         * @brief This extension point is used to redirect when a trigger is called.
         *
         * @details
         *  Normally a trigger will be called when it's own type is emitted, however it
         *  is often useful to trigger a callback when a different type is emitted.
         *  This can be used to create a type that is based on the Triggered type, but
         *  is an operation performed on it.
         *
         * @tparam TData TODO
         */
        template <typename TData>
        struct TriggerType;
    };
}

// We need to really make sure that PowerPlant is included as we use it in our ipp file
#include "nuclear_bits/Environment.h"
#include "nuclear_bits/PowerPlant.h"
#include "nuclear_bits/Reactor.ipp"
#endif

