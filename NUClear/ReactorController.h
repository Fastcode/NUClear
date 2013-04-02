#ifndef NUCLEAR_REACTORCONTROLLER_H
#define NUCLEAR_REACTORCONTROLLER_H
#include <typeindex>
#include <map>
#include <memory>
#include <vector>
#include <iostream>
#include <iterator>
#include <thread>
#include <set>

namespace NUClear {
    class Reactor;

    class ReactorController {
        public:
            friend class Reactor;

            template <typename TTrigger>
            void emit(TTrigger* data);

            template <typename TData>
            std::shared_ptr<TData> get();

            template <typename TReactor>
            void install();
        
        private:
            template <typename TTrigger>
            void subscribe(Reactor* reactor);

            template <typename TTrigger>
            void cache(TTrigger* data);

            template <typename TTrigger>
            void notifyReactors();

            template <typename TTrigger>
            std::set<Reactor*>& getReactorBindings();

            std::vector<std::unique_ptr<Reactor>> m_reactors;
            std::map<std::type_index, std::shared_ptr<void> > m_cache;
            std::map<std::type_index, std::set<Reactor*> > m_reactorBindings;
    };
}

// === Template Implementations ===
// As with Reactor.h this section contains all template function implementations for ReactorController. 
// Unfortunately it's neccecary to have these here and outside the NUClear namespace due to our circular dependency on Reactor.
// What this means is: This code needs to be down here, it can't be moved into the class or namespace since that'll break the 
// forward declaration resolution. See the similar comment in Reactor.h for more information.

#include "Reactor.h"

template <typename TReactor>
void NUClear::ReactorController::install() {
    // The reactor constructor should handle subscribing to events
    std::unique_ptr<NUClear::Reactor> reactor(new TReactor(*this));
    m_reactors.push_back(std::move(reactor));
}

template <typename TTrigger>
void NUClear::ReactorController::subscribe(Reactor* reactor) {
    std::set<Reactor*>& bindings = getReactorBindings<TTrigger>();
    bindings.insert(reactor);
}

template <typename TTrigger>
std::set<NUClear::Reactor*>& NUClear::ReactorController::getReactorBindings() {
    if(m_reactorBindings.find(typeid(TTrigger)) == m_reactorBindings.end()) {
        m_reactorBindings[typeid(TTrigger)] = std::set<NUClear::Reactor*>();
    }
    return m_reactorBindings[typeid(TTrigger)];
}

template <typename TData>
std::shared_ptr<TData> NUClear::ReactorController::get() {
    if(m_cache.find(typeid(TData)) == m_cache.end()) {
        // TODO: Better error stuff
        std::cerr << "Trying to get missing TData:" << std::endl;
    }

    return std::static_pointer_cast<TData>(m_cache[typeid(TData)]);
}

template <typename TTrigger>
void NUClear::ReactorController::emit(TTrigger* data) {
    cache<TTrigger>(data);
    notifyReactors<TTrigger>(); 
}

// == Private Methods ==
template <typename TTrigger>
void NUClear::ReactorController::cache(TTrigger* data) {
    m_cache[typeid(TTrigger)] = std::shared_ptr<void>(data);
}

template <typename TTrigger>
void NUClear::ReactorController::notifyReactors() {
    std::cout << "Notify Reactors" << std::endl;
    std::set<NUClear::Reactor*>& reactors = getReactorBindings<TTrigger>();
    for(std::set<NUClear::Reactor*>::iterator reactor = std::begin(reactors); reactor != std::end(reactors); ++reactor) {
        (*reactor)->notify<TTrigger>();
    }
}

#endif
