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

    template <typename TTrigger>
    void Reactor::notify() {
        auto& callbacks = getCallbackList<TTrigger>();
        std::cout << "Notify start" << std::endl;
        for(auto callback = std::begin(callbacks); callback != std::end(callbacks); ++callback) {
            std::cout << "Notifying" << std::endl;
            
            std::cout <<
            "Sync type: " << callback->m_options.m_syncType.name() <<
            ", Priority: " << callback->m_options.m_priority <<
            ", Single: " << callback->m_options.m_single << std::endl;
            
            // TODO threading code goes here
            (*callback)();
        }
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
            Internal::ReactionOptions options;
            if(sizeof...(TOptions) > 0) {
                context->buildOptions<TOptions...>(options);
            }
            context->bindTriggers<TTriggers...>(context->buildReaction<TFunc, TTriggers..., TWiths...>(callback, options));
    }
    
    template <typename... TOption>
    void Reactor::buildOptions(Internal::ReactionOptions& options) {
        // See unpack.h for explanation
        Internal::Magic::unpack((buildOptionsImpl(options, reinterpret_cast<TOption*>(0)), 0)...);
    }
    
    template <typename TSync>
    void Reactor::buildOptionsImpl(Internal::ReactionOptions& options, Sync<TSync>* /*placeholder*/) {
        options.m_syncType = typeid(TSync);
    }
    
    template <enum EPriority P>
    void Reactor::buildOptionsImpl(Internal::ReactionOptions& options, Priority<P>* /*placeholder*/) {
        options.m_priority = P;
    }

    template <typename TFunc, typename... TTriggersAndWiths>
    Internal::Reaction Reactor::buildReaction(TFunc callback, Internal::ReactionOptions& options) {
        
        Internal::Reaction callbackWrapper([this, callback]() -> void {
            callback((*reactorController.get<TTriggersAndWiths>())...);
        }, options);

        return Internal::Reaction(callbackWrapper);
    }

    /**
     * @brief calls bindTriggersImpl on every trigger in the list. Minimum list size is 1
     * @tparam TTrigger the required first trigger
     * @tparam TTriggers the rest of the triggers
     * @param callback the callback to bind to these triggers
     */
    template <typename TTrigger, typename... TTriggers>
    void Reactor::bindTriggers(Internal::Reaction callback) {
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
    void Reactor::bindTriggersImpl(Internal::Reaction callback, TTrigger* /*placeholder*/) {
        std::cout << "BindTriggerImpl for " << typeid(TTrigger).name() << std::endl;
        auto& callbacks = getCallbackList<TTrigger>();
        callbacks.push_back(callback);

        reactorController.reactormaster.subscribe<TTrigger>(this);
    }

    template <int ticks, class period>
    void Reactor::bindTriggersImpl(Internal::Reaction callback, Every<ticks, period>* placeholder) {
        
        // Add this interval to the chronometer
        reactorController.chronomaster.add<ticks, period>();
        
        // Register as normal
        bindTriggersImpl<Every<ticks, period>>(callback, placeholder);
    }

    /**
     * @brief Gets the callback list for a given type
     * @tparam TTrigger the type to get the callback list for
     * @returns The callback list
     */
    template <typename TTrigger>
    std::vector<NUClear::Internal::Reaction>& Reactor::getCallbackList() {
        if(m_callbacks.find(typeid(TTrigger)) == m_callbacks.end()) {
            m_callbacks[typeid(TTrigger)] = std::vector<NUClear::Internal::Reaction>();
        }

        return m_callbacks[typeid(TTrigger)];
    }
}
