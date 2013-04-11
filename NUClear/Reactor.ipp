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
            Internal::ReactionOptions options(typeid(nullptr));
            context->bindTriggers<TTriggers...>(context->buildReaction<TFunc, TTriggers..., TWiths...>(callback, context->buildOptions<TOptions...>(options)));
    }
    
    template <typename... TOption>
    Internal::ReactionOptions Reactor::buildOptions(Internal::ReactionOptions options) {
        
    }
    
    template <typename TOptionFirst, typename TOptionSecond, typename... TOptions>
    Internal::ReactionOptions Reactor::buildOptions(Internal::ReactionOptions options) {
        
    }
    
    void Reactor::buildOptionsImpl(Internal::ReactionOptions& options, Single* /*placeholder*/) {
        
    }
    
    template <typename TSync>
    void Reactor::buildOptionsImpl(Internal::ReactionOptions& options, Sync<TSync>* /*placeholder*/) {
        
    }
    
    template <enum EPriority P>
    void Reactor::buildOptionsImpl(Internal::ReactionOptions& options, Priority<P>* /*placeholder*/) {
        
    }

    template <typename TFunc, typename... TTriggersAndWiths>
    Internal::Reaction Reactor::buildReaction(TFunc callback, Internal::ReactionOptions options) {
        Internal::Reaction callbackWrapper([this, callback]() -> void {
            callback((*reactorController.get<TTriggersAndWiths>())...);
        });

        return Internal::Reaction(callbackWrapper);
    }


    /**
     * @brief Adds a single data -> callback mapping for a single type.
     * @tparam TTrigger the event/data type to add the callback for
     * @param callback the callback to add
     */
    template <typename TTrigger>
    void Reactor::bindTriggers(Internal::Reaction callback) {
        // This 0 is a nullptr however, it can't be nullptr as the cast would be invalid
        bindTriggersImpl(callback, reinterpret_cast<TTrigger*>(0));
    }

    /**
     * @brief Recursively calls the single-param bindTriggers method for every trigger in the list.
     * @tparam TTriggerFirst the next trigger to call bindTriggers on
     * @tparam TTriggerSecond the following trigger to call bindTriggers on
     * @tparam TTriggers the remaining triggers to evaluate
     * @param callback the callback to bind to all of these triggers.
     */
    template <typename TTriggerFirst, typename TTriggerSecond, typename... TTriggers>
    void Reactor::bindTriggers(Internal::Reaction callback) {
        bindTriggers<TTriggerFirst>(callback);
        bindTriggers<TTriggerSecond, TTriggers...>(callback);
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
