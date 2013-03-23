#ifndef NUCLEAR_REACTOR_H
#define NUCLEAR_REACTOR_H
#include <functional>
#include <typeindex>
#include <vector>
#include <map>
#include <iostream>
#include "Reaction.h"
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
        private:
            template <typename TTrigger, typename... TWith>
            class OnCallBinder {
                public:
                    OnCallBinder(Reactor* parent, 
                            ReactorController& reactorControl, 
                            std::vector<std::function<void ()>>& callbackList) :
                        m_parent(parent),
                        m_reactorControl(reactorControl),
                        m_callbackList(callbackList) {
                    }

                    template <typename TFunc>
                    void reactWith(TFunc callback);
                private:
                    Reactor* m_parent;
                    ReactorController& m_reactorControl;
                    std::vector<std::function<void ()>>& m_callbackList;
            };

        public:
            Reactor();
            ~Reactor();

            /**
             * @brief Triggers the callbacks for a given event.
             * @tparam TTrigger the type of event to trigger.
             */
            template <typename TTrigger>
            void trigger(reactionId_t parentId);
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

            template <typename TTrigger, typename... TWith>
            OnCallBinder<TTrigger, TWith...> on();

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
void NUClear::Reactor::trigger(reactionId_t parentId) {
    auto& callbacks = getCallbackList<TTrigger>();
    for(auto callback = std::begin(callbacks); callback != std::end(callbacks); ++callback) {
        
        // Build up our task object
        this->reactorControl.submit(std::unique_ptr<Reaction>(new Reaction(*callback, typeid(TTrigger), parentId)));
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

template <typename TTrigger, typename... TWith>
typename NUClear::Reactor::OnCallBinder<TTrigger, TWith...> NUClear::Reactor::on() {
    return NUClear::Reactor::OnCallBinder<TTrigger, TWith...>(this, this->reactorControl, getCallbackList<TTrigger>());
}

template <typename TTrigger, typename... TWith>
template <typename TFunc>
void NUClear::Reactor::OnCallBinder<TTrigger, TWith...>::reactWith(TFunc callback) {
    m_callbackList.push_back([this, &callback]() {
        callback( 
            // Because get<...> is dependant on reactorControl which is 
            // dependant on m_parent which is dependant on OnCallBinder
            // which is dependant on TTrigger and TWith we have what's known
            // in hell as a "type-dependant expression". This means the compiler
            // doesn't know if get<...> is an expression using operator <, > and ().
            // so we need to tell it by prefixing the call with template. Don't you
            // just love C++! 
            // see: http://stackoverflow.com/a/613132/203133
            // and: http://stackoverflow.com/a/1090850/203133
            *(m_reactorControl.template get<TTrigger>()), 
            *(m_reactorControl.template get<TWith>())... 
        );
    });
    m_reactorControl.addReactor<TTrigger>(*m_parent);
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
