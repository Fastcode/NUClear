#ifndef NUCLEAR_REACTOR_H
#define NUCLEAR_REACTOR_H
#include <functional>
#include <typeindex>
#include <vector>
#include <map>
#include <iostream>
namespace NUClear {

    class ReactorController;

    /**
     * @brief Base class for all systems that want to recieve data from the system.
     * @details Provides the ability to register callbacks for incoming events.
     * @author Jake Woods
     * @version 1.0
     * @date 19-Mar-2013
     */
    class Reactor {
        public:
            Reactor();

            /**
             * @brief Triggers the callbacks for a given event.
             * @tparam TTrigger the type of event to trigger.
             */
            template <typename TTrigger>
            void trigger();
        protected:
            /**
             * @brief Registers a callback for a given event.
             * @tparam TTrigger the type of event to trigger on.
             * @tparam TWith the events that the callback needs access to on triggering.
             * @param callback The callback to call when the event is triggered.
             */
            template <typename TTrigger, typename... TWith>
            void reactOn(void callback(const TTrigger&, const TWith&...)); 

            /**
             * @brief Registers a callback for a given event using a magic name.
             * @tparam ChildType the concrete type of the reaction class. (ex, in vision: reactOn<Vision, TTrigger, TWith...>() )
             * @tparam TTrigger the type of event to trigger on
             * @tparam TWith the events that the callback needs access to on triggering.
             * @pre react(const TTrigger&, const TWith&...) is defined in the child class
             * @remarks 
             *     this overload of reactOn acts similarly to the parameterized callback
             *     except it automatically looks for a function named "react" with the
             *     appropriate parameters. It will then automatically bind that react
             *     function. If no such function is found you'll get a horrible compile
             *     error.
             */
            template <typename ChildType, typename TTrigger, typename... TWith>
            void reactOn(); 

        private:
            ReactorController& reactorControl;

            std::map<std::type_index, std::vector<std::function<void ()>>> m_reactions;

            /**
             * @brief Returns a list of callbacks for a given type.
             * @tparam TTrigger the event type to get a list of callbacks for
             * @return the list of callbacks.
             */
            template <typename TTrigger>
            std::vector<std::function<void ()>>& getCallbackList();
    };

}

// === Template Implementations ===
// This section contains all the implementations for the Reactor functions. Unfortunately it's neccecary to have these
// outside of the NUClear namespace and as a seperate implementation as we need to resolve the forward
// declaration of ReactorController so we can actually use the reference. What this means is: This code needs
// to be down here, it can't be moved into the class or namespace since that'll break the forward declaration resolution.
// Welcome to the unfortunate reality of the C++ include system (what I wouldn't give for modules...)
#include "ReactorController.h"

// == Public Methods ==
template <typename TTrigger>
void NUClear::Reactor::trigger() {
    auto& callbacks = getCallbackList<TTrigger>();
    for(auto callback = std::begin(callbacks); callback != std::end(callbacks); ++callback) {
        (*callback)();
    }
}

// == Protected Methods == 
template <typename TTrigger, typename... TWith>
void NUClear::Reactor::reactOn(void callback(const TTrigger&, const TWith&...)) {
    auto& reactors = getCallbackList<TTrigger>();
    reactors.push_back([this, callback]() {
        callback( *(this->reactorControl.get<TTrigger>()), *(this->reactorControl.get<TWith>())... );
    });
    reactorControl.addReactor<TTrigger>(*this);
}

template <typename ChildType, typename TTrigger, typename... TWith>
void NUClear::Reactor::reactOn() {
    auto& reactors = getCallbackList<TTrigger>();
    reactors.push_back([this]() {
        static_cast<ChildType*>(this)->react( (*reactorControl.get<TTrigger>()), (*reactorControl.get<TWith>())... );
    });
    reactorControl.addReactor<TTrigger>(*this);
}

// == Private Methods ==
template <typename TTrigger>
std::vector<std::function<void ()>>& NUClear::Reactor::getCallbackList() {
    if(m_reactions.find(typeid(TTrigger)) == m_reactions.end()) {
        // Creates a new std::vector<...> for this callback. This means
        // that if there are no callbacks then an empty list will
        // be returned instead of an exception!
        m_reactions[typeid(TTrigger)] = std::vector<std::function<void ()>>();
    }

    return m_reactions[typeid(TTrigger)];
}
#endif
