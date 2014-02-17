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

namespace NUClear {
    
    // This metafunction builds a proper On given a series of function arguments
    template <typename TFunc, typename... TParams>
    struct Reactor::On : public OnBuilder<TFunc, Reactor::Trigger<>, Reactor::With<>, Reactor::Options<>, std::tuple<>, TParams...> {};
    
    template <
    typename TFunc,
    typename... TTriggers,
    typename... TWiths,
    typename... TOptions,
    typename... TFuncArgs,
    typename... TNewTriggers,
    typename... TParams>
    struct Reactor::OnBuilder<
    TFunc,
    Reactor::Trigger<TTriggers...>,
    Reactor::With<TWiths...>,
    Reactor::Options<TOptions...>,
    std::tuple<TFuncArgs...>,
    Reactor::Trigger<TNewTriggers...>,
    TParams...> :
    public Reactor::OnBuilder<
    TFunc,
    Reactor::Trigger<TTriggers..., TNewTriggers...>,
    Reactor::With<TWiths...>,
    Reactor::Options<TOptions...>,
    std::tuple<TFuncArgs..., TNewTriggers...>,
    TParams...> {};
    
    template <
    typename TFunc,
    typename... TTriggers,
    typename... TWiths,
    typename... TOptions,
    typename... TFuncArgs,
    typename... TNewWiths,
    typename... TParams>
    struct Reactor::OnBuilder<
    TFunc,
    Reactor::Trigger<TTriggers...>,
    Reactor::With<TWiths...>,
    Reactor::Options<TOptions...>,
    std::tuple<TFuncArgs...>,
    Reactor::With<TNewWiths...>,
    TParams...> :
    public Reactor::OnBuilder<
    TFunc,
    Reactor::Trigger<TTriggers...>,
    Reactor::With<TWiths..., TNewWiths...>,
    Reactor::Options<TOptions...>,
    std::tuple<TFuncArgs..., TNewWiths...>,
    TParams...> {};
    
    template <
    typename TFunc,
    typename... TTriggers,
    typename... TWiths,
    typename... TOptions,
    typename... TFuncArgs,
    typename... TNewOptions,
    typename... TParams>
    struct Reactor::OnBuilder<
    TFunc,
    Reactor::Trigger<TTriggers...>,
    Reactor::With<TWiths...>,
    Reactor::Options<TOptions...>,
    std::tuple<TFuncArgs...>,
    Reactor::Options<TNewOptions...>,
    TParams...> :
    public Reactor::OnBuilder<
    TFunc,
    Reactor::Trigger<TTriggers...>,
    Reactor::With<TWiths...>,
    Reactor::Options<TOptions..., TNewOptions...>,
    std::tuple<TFuncArgs...>,
    TParams...> {};
    
    template <
    typename TFunc,
    typename... TTriggers,
    typename... TWiths,
    typename... TOptions,
    typename... TFuncArgs>
    struct Reactor::OnBuilder<
    TFunc,
    Reactor::Trigger<TTriggers...>,
    Reactor::With<TWiths...>,
    Reactor::Options<TOptions...>,
    std::tuple<TFuncArgs...>> :
    public Reactor::On<
    TFunc,
    Reactor::Trigger<TTriggers...>,
    Reactor::With<TWiths...>,
    Reactor::Options<TOptions...>,
    std::tuple<TFuncArgs...>> {};
    
    template <typename TFunc, typename... TTypes>
    struct Reactor::CheckFunctionSignature<TFunc, std::tuple<TTypes...>, 0> :
    public Reactor::CheckFunctionSignature<TFunc, std::tuple<decltype(std::declval<PowerPlant::CacheMaster>().get<TTypes>())...>, 1> {};
    
    template <typename TFunc, typename... TTypes>
    struct Reactor::CheckFunctionSignature<TFunc, std::tuple<TTypes...>, 1> :
    public Reactor::CheckFunctionSignature<TFunc, std::tuple<decltype(metaprogramming::Dereferenceable<TTypes>::dereference(std::declval<TTypes>()))...>, 2> {};
    
    template <typename TFunc, typename... TTypes>
    struct Reactor::CheckFunctionSignature<TFunc, std::tuple<TTypes...>, 2> :
    public Reactor::CheckFunctionSignature<TFunc, std::tuple<Meta::AddConst<Meta::RemoveRef<TTypes>>...>, 3> {};
    
    template <typename TFunc, typename... TTypes>
    struct Reactor::CheckFunctionSignature<TFunc, std::tuple<TTypes...>, 3> :
    public Meta::If<std::is_assignable<std::function<void (TTypes...)>, TFunc>, std::true_type, std::false_type> {};
    
    
    /* End Meta functions */
    
    /**
     * @brief Demangles the passed symbol to a string, or returns it if it cannot demangle it
     *
     * @param symbol the symbol to demangle
     *
     * @return the demangled symbol, or the original string if it could not be demangeld
     */
    inline std::string demangle(const char* symbol) {
        
        int status = -4; // some arbitrary value to eliminate the compiler warning
        std::unique_ptr<char, void(*)(void*)> res {
            abi::__cxa_demangle(symbol, nullptr, nullptr, &status),
            std::free
        };
        
        return std::string(status == 0 ? res.get() : symbol);
    }
    
    template <typename... TParams, typename TFunc>
    Reactor::ReactionHandle Reactor::on(TFunc callback) {
        
        // Forward an empty string as our user reaction name
        return on<TParams...>("", std::forward<TFunc>(callback));
    }
    
    template <typename... TParams, typename TFunc>
    Reactor::ReactionHandle Reactor::on(const std::string& name, TFunc callback) {
        
        // There must be some parameters
        static_assert(sizeof...(TParams) > 0, "You must have at least one paramter in an on");
        
        std::stringstream identifier;
        
        identifier << name << std::endl;
        identifier << demangle(typeid(std::tuple<TParams...>).name()) << std::endl;
        identifier << demangle(typeid(TFunc).name());
        
        return On<TFunc, TParams...>::on(*this, identifier.str(), callback);
    }
    
    template <typename... THandlers, typename TData>
    void Reactor::emit(std::unique_ptr<TData>&& data) {
        powerPlant.emit<THandlers...>(std::forward<std::unique_ptr<TData>>(data));
    }
    
    // This is our final On statement
    template <typename TFunc, typename... TTriggers, typename... TWiths, typename... TOptions, typename... TFuncArgs>
    Reactor::ReactionHandle Reactor::On<
    TFunc,
    Reactor::Trigger<TTriggers...>,
    Reactor::With<TWiths...>,
    Reactor::Options<TOptions...>,
    std::tuple<TFuncArgs...>>::on(Reactor& context, std::string name, TFunc callback) {
        
        static_assert(Reactor::CheckFunctionSignature<TFunc, std::tuple<TFuncArgs...>>::value, "Your callback function does not match the types in the On statement");
        static_assert(sizeof...(TTriggers) > 0, "You must have at least one Trigger in a callback");
        
        // Build up our options
        threading::ReactionOptions options;
        context.buildOptions<TOptions...>(options);
        
        // Bind all of our trigger events to a reaction
        auto onHandler = context.bindTriggers<TTriggers...>(context.buildReaction<TFunc, TFuncArgs...>(name,
        callback, options));
        
        // Run any existence commands needed (running because a type exists)
        metaprogramming::unpack((Exists<TFuncArgs>::exists(context), 0)...);
        
        return onHandler;
    }
    
    template <typename... TOption>
    void Reactor::buildOptions(threading::ReactionOptions& options) {
        // See unpack.h for explanation
        metaprogramming::unpack((buildOptionsImpl(options, reinterpret_cast<TOption*>(0)), 0)...);
    }
    
    template <typename TSync>
    void Reactor::buildOptionsImpl(threading::ReactionOptions& options, Sync<TSync>* /*placeholder*/) {
        options.syncQueue = threading::SyncQueueFor<TSync>::queue;
    }
    
    template <enum EPriority P>
    void Reactor::buildOptionsImpl(threading::ReactionOptions& options, Priority<P>* /*placeholder*/) {
        options.priority = P;
    }
    
    template <typename TFunc, typename... TTriggersAndWiths>
    std::unique_ptr<threading::Reaction> Reactor::buildReaction(std::string name, TFunc callback, threading::ReactionOptions& options) {
        
        // Return a reaction object that gets and runs with the correct paramters
        return std::make_unique<threading::Reaction>(name, [this, callback]() -> std::function<void (threading::ReactionTask&)> {
            
            // TODO consider potential threading implications if two threads emit at once and overwrite (then this will get the same value for both)
            auto data = std::make_tuple(powerPlant.cachemaster.get<TTriggersAndWiths>()...);
            
            // TODO metafunction that when using get gets the new data object somehow?
            
            return [this, callback, data] (threading::ReactionTask& task) {
                
                this->powerPlant.threadmaster.setCurrentTask(&task);
                
                metaprogramming::apply(callback, data);
                
                this->powerPlant.threadmaster.setCurrentTask(nullptr);
            };
        }, options);
    }
    
    template <typename TData>
    struct Reactor::Exists {
        static void exists(Reactor& context) {};
    };
    
    template <typename TData>
    struct Reactor::TriggerType {
        using type = TData;
    };
    
    /**
     * @brief calls bindTriggersImpl on every trigger in the list. Minimum list size is 1
     * @tparam TTrigger the required first trigger
     * @tparam TTriggers the rest of the triggers
     * @param callback the callback to bind to these triggers
     */
    template <typename TTrigger, typename... TTriggers>
    Reactor::ReactionHandle Reactor::bindTriggers(std::unique_ptr<threading::Reaction>&& callback) {
        
        // Single trigger that is not ignored
        if(sizeof...(TTriggers) == 0 && !std::is_same<typename TriggerType<TTrigger>::type, std::nullptr_t>::value) {
            CallbackCache<typename TriggerType<TTrigger>::type>::get().push_back(std::forward<std::unique_ptr<threading::Reaction>>(callback));
        }
        
        // Multi Trigger (and)
        else {
            // TODO Generate a sequence
            // TODO set a callback for each of TTrigger and TTriggers along with their number
            // TODO have a tuple of atomic booleans for each TTrigger
            // TODO when a trigger comes in, use the int to set that boolean to true
            // TODO when all the booleans are true, run the callback and set all booleans to false
        }
        
        return ReactionHandle(callback.get());
    }
}
