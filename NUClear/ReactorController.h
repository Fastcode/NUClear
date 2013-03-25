#ifndef NUCLEAR_REACTORCONTROLLER_H
#define NUCLEAR_REACTORCONTROLLER_H
#include <typeindex>
#include <map>
#include <memory>
#include <vector>
#include <iostream>
#include <iterator>
#include <thread>
#include "ReactorCore.h"
#include "TimeEmitter.h"
namespace NUClear {
    class Reactor;

    class ReactorController {
        public:
            ReactorController();
            ~ReactorController(); 

            template <typename TTrigger>
            void emit(TTrigger* data);

            template <typename TTrigger>
            std::shared_ptr<TTrigger> get();

            template <typename TTrigger>
            void addReactor(Reactor& reactor);
        
            void submit(std::unique_ptr<Reaction>&& reaction);
        
            void addEvery(std::chrono::nanoseconds time, std::function<void ()> callback);

            void shutdown();
            void waitForThreadCompletion();
        private:
            template <typename TTrigger>
            void cache(TTrigger* data);

            template <typename TTrigger>
            void triggerReactors(reactionId_t parentId);

            template <typename TTrigger>
            std::vector<Reactor*>& getReactors();

            std::map<std::type_index, std::shared_ptr<void> > m_cache;
            std::map<std::type_index, std::vector<Reactor*> > m_reactors;

            ReactorCore core;
            TimeEmitter timeEmitter;
    };

    // Forgive me
    extern ReactorController ReactorControl;
}

// === Template Implementations ===
// As with Reactor.h this section contains all template function implementations for ReactorController. 
// Unfortunately it's neccecary to have these here and outside the NUClear namespace due to our circular dependency on Reactor.
// What this means is: This code needs to be down here, it can't be moved into the class or namespace since that'll break the 
// forward declaration resolution. See the similar comment in Reactor.h for more information.

#include "Reactor.h"

// == Public Methods ==
template <typename TTrigger>
void NUClear::ReactorController::emit(TTrigger* data) {
    
    std::cerr << "Emitting" << std::endl;
    reactionId_t parentId = core.getCurrentReactionId(std::this_thread::get_id());
    
    cache<TTrigger>(data);
    triggerReactors<TTrigger>(parentId);
}

template <typename TTrigger>
std::shared_ptr<TTrigger> NUClear::ReactorController::get() {
    if(m_cache.find(typeid(TTrigger)) == m_cache.end()) {
        std::cerr << "Trying to get missing TTrigger" << std::endl;
    }

    return std::static_pointer_cast<TTrigger>(m_cache[typeid(TTrigger)]);
}

template <typename TTrigger>
void NUClear::ReactorController::addReactor(Reactor& reactor) {
    auto& reactors = getReactors<TTrigger>();
    reactors.push_back(&reactor);
}

// == Private Methods ==
template <typename TTrigger>
void NUClear::ReactorController::cache(TTrigger* data) {
    m_cache[typeid(TTrigger)] = std::shared_ptr<void>(data);
}

template <typename TTrigger>
void NUClear::ReactorController::triggerReactors(reactionId_t parentId) {
    std::vector<NUClear::Reactor*>& reactors = getReactors<TTrigger>();
    for(auto reactor = std::begin(reactors); reactor != std::end(reactors); ++reactor) {
        (*reactor)->trigger<TTrigger>(parentId);
    }
}

template <typename TTrigger>
std::vector<NUClear::Reactor*>& NUClear::ReactorController::getReactors() {
    if(m_reactors.find(typeid(TTrigger)) == m_reactors.end()) {
        m_reactors[typeid(TTrigger)] = std::vector<NUClear::Reactor*>();
    }
    return m_reactors[typeid(TTrigger)];
}

#endif
