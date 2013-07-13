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
    
    template <typename TTrigger, typename TFunc>
    void Reactor::on(TFunc callback) {
        On<TTrigger, With<>, Options<>, TFunc>::on(this, callback);
    }

    template <typename TTrigger, typename TWith, typename TFunc>
    void Reactor::on(TFunc callback) {
        On<TTrigger, TWith, Options<>, TFunc>::on(this, callback);
    }

    template <typename TTrigger, typename TWith, typename TOptions, typename TFunc>
    void Reactor::on(TFunc callback) {
        On<TTrigger, TWith, TOptions, TFunc>::on(this, callback);
    }
    
    template <typename... THandlers, typename TData>
    void Reactor::emit(TData* data) {
        powerPlant.emit<THandlers...>(data);
    }

    // == Private Method == 

    template <typename... TTriggers, typename... TWiths, typename... TOptions, typename TFunc>
    void Reactor::On<
        Reactor::Trigger<TTriggers...>
        , Reactor::With<TWiths...>
        , Reactor::Options<TOptions...>
        , TFunc>::on(Reactor* context, TFunc callback) {
            
            // Build up our options
            Internal::Reaction::Options options;
            context->buildOptions<TOptions...>(options);
            
            // Run any existence commands needed (running because a type exists)
            Internal::Magic::unpack((Exists<TTriggers>::exists(context), 0)...);
            Internal::Magic::unpack((Exists<TWiths>::exists(context), 0)...);
            
            // Bind all of our trigger events
            context->bindTriggers<TTriggers...>(context->buildReaction<TFunc, TTriggers..., TWiths...>(callback, options));
    }
    
    template <typename... TOption>
    void Reactor::buildOptions(Internal::Reaction::Options& options) {
        // See unpack.h for explanation
        Internal::Magic::unpack((buildOptionsImpl(options, reinterpret_cast<TOption*>(0)), 0)...);
    }
    
    template <typename TSync>
    void Reactor::buildOptionsImpl(Internal::Reaction::Options& options, Sync<TSync>* /*placeholder*/) {
        options.m_syncQueue = Sync<TSync>::SyncQueue;
    }
    
    template <enum EPriority P>
    void Reactor::buildOptionsImpl(Internal::Reaction::Options& options, Priority<P>* /*placeholder*/) {
        options.m_priority = P;
    }
    
    // NEEDS FILL METAFUNCTION
    template <typename... TTypes>
    struct NeedsFill : std::false_type {};
    
    template <typename... TTypes>
    struct NeedsFill<std::tuple<TTypes...>> : NeedsFill<TTypes...> {};
    
    template <typename THead, typename... TTypes>
    struct NeedsFill<THead, TTypes...> :
    std::conditional<
        std::is_base_of<Internal::CommandTypes::FillType, THead>::value,
        std::true_type,
        NeedsFill<TTypes...>
    >::type {};
    
    template <>
    struct Reactor::buildCallback<true> {
        template <typename TFunc, typename... TData>
        static const std::function<void ()> get(Reactor* parent, TFunc callback, std::tuple<TData...> data) {
            return [parent, callback, data] {
                
                // Perform our second pass over the data
                auto&& newData = parent->powerPlant.cachemaster.fill(data);
                
                // TODO do a static assert that TFunc can take arguments of type decltype(newData)
                
                parent->powerPlant.cachemaster.setThreadArgs(std::this_thread::get_id(), std::move(Internal::Magic::buildVector(newData)));
                
                Internal::Magic::apply(callback, std::move(newData));
            };
        }
    };
    
    template <>
    struct Reactor::buildCallback<false> {
        
        template <typename TFunc, typename... TData>
        static const std::function<void ()> get(Reactor* parent, TFunc callback, std::tuple<TData...> data) {
            return [parent, callback, data] {
                
                // TODO do a static assert that TFunc can take arguments of type decltype(data)
                
                parent->powerPlant.cachemaster.setThreadArgs(std::this_thread::get_id(), std::move(Internal::Magic::buildVector(data)));
                
                Internal::Magic::apply(callback, std::move(data));
            };
        }
    };

    template <typename TFunc, typename... TTriggersAndWiths>
    Internal::Reaction* Reactor::buildReaction(TFunc callback, Internal::Reaction::Options& options) {
        
        // Return a reaction object that gets and runs with the correct paramters
        return new Internal::Reaction(typeid(TFunc).name(), [this, callback]() -> std::function<void ()> {
            
            auto&& data = std::make_tuple(powerPlant.get<TTriggersAndWiths>()...);
            
            return buildCallback<NeedsFill<typename std::remove_reference<decltype(data)>::type>::value>::get(this, callback, data);
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
    void Reactor::bindTriggers(Internal::Reaction* callback) {
        
        // Single trigger that is not ignored
        if(sizeof...(TTriggers) == 0 && !std::is_same<typename TriggerType<TTrigger>::type, std::nullptr_t>::value) {
            CallbackCache<typename TriggerType<TTrigger>::type>::set(callback);
        }
        
        // Multi Trigger (and)
        else {
            // TODO Generate a sequence
            // TODO set a callback for each of TTrigger and TTriggers along with their number
            // TODO have a tuple of atomic booleans for each TTrigger
            // TODO when a trigger comes in, use the int to set that boolean to true
            // TODO when all the booleans are true, run the callback and set all booleans to false
        }
    }
}
