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
        OnImpl<TTrigger, With<>, Options<>, TFunc>(this)(callback);
    }

    template <typename TTrigger, typename TWith, typename TFunc>
    void Reactor::on(TFunc callback) {
        OnImpl<TTrigger, TWith, Options<>, TFunc>(this)(callback);
    }

    template <typename TTrigger, typename TWith, typename TOptions, typename TFunc>
    void Reactor::on(TFunc callback) {
        OnImpl<TTrigger, TWith, TOptions, TFunc>(this)(callback);
    }

    // == Private Method == 
    template <typename... TTriggers, typename... TWiths, typename... TOptions, typename TFunc>
    Reactor::OnImpl<
        Reactor::Trigger<TTriggers...>
        , Reactor::With<TWiths...>
        , Reactor::Options<TOptions...>
        , TFunc>::OnImpl(Reactor* context) {
            this->context = context;
    }

    template <typename... TTriggers, typename... TWiths, typename... TOptions, typename TFunc>
    void Reactor::OnImpl<
        Reactor::Trigger<TTriggers...>
        , Reactor::With<TWiths...>
        , Reactor::Options<TOptions...>
        , TFunc>::operator()(TFunc callback) {
            Internal::Reaction::Options options;
            context->bindTriggers<TTriggers...>(context->buildReaction<TFunc, TTriggers..., TWiths...>(callback, options));
    }
    
    template <typename... TOption>
    void Reactor::buildOptions(Internal::Reaction::Options& options) {
        // See unpack.h for explanation
        Internal::Magic::unpack((buildOptionsImpl(options, reinterpret_cast<TOption*>(0)), 0)...);
    }
    
    template <typename TSync>
    void Reactor::buildOptionsImpl(Internal::Reaction::Options& options, Sync<TSync>* /*placeholder*/) {
        options.m_syncType = typeid(TSync);
    }
    
    template <enum EPriority P>
    void Reactor::buildOptionsImpl(Internal::Reaction::Options& options, Priority<P>* /*placeholder*/) {
        options.m_priority = P;
    }
    
    template <typename... TTypes>
    struct NeedsSecondPassImpl;
    
    template <>
    struct NeedsSecondPassImpl<> : std::false_type {};
    
    template <typename THead, typename... TRest>
    struct NeedsSecondPassImpl<THead, TRest...> : std::conditional<std::is_base_of<Internal::CommandTypes::SecondPass, THead>::value, std::true_type, NeedsSecondPassImpl<TRest...>>::type {};
    
    template <typename TTuple>
    struct NeedsSecondPass;
    
    template <typename... TTypes>
    struct NeedsSecondPass<std::tuple<TTypes...>> : NeedsSecondPassImpl<TTypes...> {
    };
    
    template <>
    struct Reactor::buildCallback<true> {
        template <typename TFunc, typename... TData>
        static const std::function<void ()> get(Reactor* parent, TFunc callback, std::tuple<TData...> data) {
            return [parent, callback, data] {
                auto&& newData = parent->reactorController.cachemaster.link(data);
                Internal::Magic::apply(callback, std::move(newData));
            };
        }
    };
    
    template <>
    struct Reactor::buildCallback<false> {
        
        template <typename TFunc, typename... TData>
        static const std::function<void ()> get(Reactor* parent, TFunc callback, std::tuple<TData...> data) {
            return [parent, callback, data] {
                Internal::Magic::apply(callback, std::move(data));
            };
        }
    };

    template <typename TFunc, typename... TTriggersAndWiths>
    Internal::Reaction* Reactor::buildReaction(TFunc callback, Internal::Reaction::Options& options) {
        
        // Return a reaction object that gets and runs with the correct paramters
        return new Internal::Reaction([this, callback]() -> std::function<void ()> {
            
            auto&& data = std::make_tuple(reactorController.get<TTriggersAndWiths>()...);
            
            return buildCallback<NeedsSecondPass<typename std::remove_reference<decltype(data)>::type>::value>::get(this, callback, data);
        }, options);
    }

    /**
     * @brief calls bindTriggersImpl on every trigger in the list. Minimum list size is 1
     * @tparam TTrigger the required first trigger
     * @tparam TTriggers the rest of the triggers
     * @param callback the callback to bind to these triggers
     */
    template <typename TTrigger, typename... TTriggers>
    void Reactor::bindTriggers(Internal::Reaction* callback) {
        bindTriggersImpl(callback, reinterpret_cast<TTrigger*>(0));

        // See unpack.h for explanation
        Internal::Magic::unpack((bindTriggersImpl(callback, reinterpret_cast<TTriggers*>(0)), 0)...);
    }

    /**
     * @brief The implementation method for bindTriggers, provides partial template specialization for specific-trigger type logic.
     * @tparam TTrigger the trigger to bind to
     * @param callback the callback to bind
     * @param placeholder used for partial template specialization
     */
    template <typename TTrigger>
    void Reactor::bindTriggersImpl(Internal::Reaction* callback, TTrigger* /*placeholder*/) {
        
        // Store our data in the cache
        CallbackCache<TTrigger>::set(callback);
    }

    template <int ticks, class period>
    void Reactor::bindTriggersImpl(Internal::Reaction* callback, Every<ticks, period>* placeholder) {
        
        // Add this interval to the chronometer
        reactorController.chronomaster.add<ticks, period>();
        
        // Register as normal
        bindTriggersImpl<Every<ticks, period>>(callback, placeholder);
    }
    
    template <int num, typename TData>
    void Reactor::bindTriggersImpl(Internal::Reaction* callback, Last<num, TData>* placeholder) {
        
        // Let the ReactorMaster know to cache at least this many of this type
        reactorController.cachemaster.ensureCache<num, TData>();
        
        // Register our callback on the inner type
        bindTriggersImpl<TData>(callback, reinterpret_cast<TData*>(0));
    }
}
