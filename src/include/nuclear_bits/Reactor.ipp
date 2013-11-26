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
        TFunc, Reactor::Trigger<TTriggers...>,
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
    public Reactor::CheckFunctionSignature<TFunc, std::tuple<decltype(Internal::Magic::Dereferenceable<TTypes>::dereference(std::declval<TTypes>()))...>, 2> {};
    
    template <typename TFunc, typename... TTypes>
    struct Reactor::CheckFunctionSignature<TFunc, std::tuple<TTypes...>, 2> :
    public Reactor::CheckFunctionSignature<TFunc, std::tuple<Meta::AddConst<Meta::RemoveRef<TTypes>>...>, 3> {};
    
    template <typename TFunc, typename... TTypes>
    struct Reactor::CheckFunctionSignature<TFunc, std::tuple<TTypes...>, 3> :
    public Meta::If<std::is_assignable<std::function<void (TTypes...)>, TFunc>, std::true_type, std::false_type> {};
    
    
    /* End Meta functions */
    template <enum LogLevel level, typename... TArgs>
    void Reactor::log(TArgs... args) {
        if(level >= this->environment->logLevel) {
            // Build our log
            std::stringstream outputStream;
            logImpl(outputStream, std::forward<TArgs>(args)...);
            std::string output = outputStream.str();

            // Attach our log to the current reaction.
            auto task = powerPlant->threadmaster.getCurrentTask(std::this_thread::get_id());

            // Task could be null if log is called from a non-reaction context.
            // If so we just don't link it to the current reaction.
            if(task != nullptr) {
                task->stats->log.push_back(output);
            }

            emit<Scope::DIRECT>(std::make_unique<Messages::LogMessage>(output));
        }
    }

    template <typename TFirst, typename... TArgs>
    void Reactor::logImpl(std::stringstream& output, TFirst first, TArgs... args) {
        output << first;
        logImpl(output, std::forward<TArgs>(args)...);
    }

    template <typename TFirst>
    void Reactor::logImpl(std::stringstream& output, TFirst first) {
        output << first;
    }

    template <typename... TParams, typename TFunc>
    void Reactor::on(TFunc callback) {
        
        // There must be some parameters
        static_assert(sizeof...(TParams) > 0, "TODO not enough parameters message");
        
        On<TFunc, TParams...>::on(this, callback);
    }
    
    template <typename... THandlers, typename TData>
    void Reactor::emit(std::unique_ptr<TData>&& data) {
        powerPlant->emit<THandlers...>(std::forward<std::unique_ptr<TData>>(data));
    }
    
    // This is our final On statement
    template <typename TFunc, typename... TTriggers, typename... TWiths, typename... TOptions, typename... TFuncArgs>
    Reactor::OnHandle Reactor::On<
        TFunc,
        Reactor::Trigger<TTriggers...>,
        Reactor::With<TWiths...>,
        Reactor::Options<TOptions...>,
        std::tuple<TFuncArgs...>>::on(Reactor* context, TFunc callback) {
            
            static_assert(Reactor::CheckFunctionSignature<TFunc, std::tuple<TFuncArgs...>>::value, "Your callback function does not match the types in the On statement");
            static_assert(sizeof...(TTriggers) > 0, "You must have at least one Trigger in a callback");
            
            // Build up our options
            Internal::Reaction::Options options;
            context->buildOptions<TOptions...>(options);
            
            
            // Bind all of our trigger events to a reaction
            auto onHandler = context->bindTriggers<TTriggers...>(context->buildReaction<TFunc, TFuncArgs...>(callback, options));

            // Run any existence commands needed (running because a type exists)
            Internal::Magic::unpack((Exists<TFuncArgs>::exists(context), 0)...);

            return onHandler;
    }
    
    template <typename... TOption>
    void Reactor::buildOptions(Internal::Reaction::Options& options) {
        // See unpack.h for explanation
        Internal::Magic::unpack((buildOptionsImpl(options, reinterpret_cast<TOption*>(0)), 0)...);
    }
    
    template <typename TSync>
    void Reactor::buildOptionsImpl(Internal::Reaction::Options& options, Sync<TSync>* /*placeholder*/) {
        options.syncQueue = Sync<TSync>::queue;
    }
    
    template <enum EPriority P>
    void Reactor::buildOptionsImpl(Internal::Reaction::Options& options, Priority<P>* /*placeholder*/) {
        options.priority = P;
    }

    template <typename TFunc, typename... TTriggersAndWiths>
    std::unique_ptr<Internal::Reaction> Reactor::buildReaction(TFunc callback, Internal::Reaction::Options& options) {
        
        // Return a reaction object that gets and runs with the correct paramters
        return std::make_unique<Internal::Reaction>(typeid(TFunc).name(), [this, callback]() -> std::function<void (Internal::Reaction::Task&)> {

            // TODO consider potential threading implications if two threads emit at once and overwrite (then this will get the same value for both)
            auto&& data = std::make_tuple(powerPlant->cachemaster.get<TTriggersAndWiths>()...);
            
            return [this, callback, data] (Internal::Reaction::Task& task) {
                
                task.args = Internal::Magic::buildVector(data);
                this->powerPlant->threadmaster.setCurrentTask(std::this_thread::get_id(), &task);
                
                Internal::Magic::apply(callback, data);
                
                this->powerPlant->threadmaster.setCurrentTask(std::this_thread::get_id(), nullptr);
            };
        }, options);
    }
    
    template <typename TData>
    struct Reactor::Exists {
        static void exists(Reactor* context) {};
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
    Reactor::OnHandle Reactor::bindTriggers(std::unique_ptr<Internal::Reaction>&& callback) {
        
        // Single trigger that is not ignored
        if(sizeof...(TTriggers) == 0 && !std::is_same<typename TriggerType<TTrigger>::type, std::nullptr_t>::value) {
            CallbackCache<typename TriggerType<TTrigger>::type>::get().push_back(std::forward<std::unique_ptr<Internal::Reaction>>(callback));
        }
        
        // Multi Trigger (and)
        else {
            // TODO Generate a sequence
            // TODO set a callback for each of TTrigger and TTriggers along with their number
            // TODO have a tuple of atomic booleans for each TTrigger
            // TODO when a trigger comes in, use the int to set that boolean to true
            // TODO when all the booleans are true, run the callback and set all booleans to false
        }
        
        return OnHandle(callback.get());
    }
}
