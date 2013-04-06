// Here lies yet another artifact of the terrible C++ "module" system, and I use that term sparingly. 
// Reactor.h needs to be included before ReactorController in all contexts, this is neccecary because of the
// three class circular dependency between [ReactorController -> ReactorMaster -> Reactor]. By forcing Reactor.h
// here we can ensure that the defintion of Reactor will exist for ReactorMaster avoiding compile time errors
// and making the whole mess work. If C++ had a module system this would be a non-issue but we're looking at
// sometime between 2014-2017 to get one of those! It needs to be outside of the macro guards because 
// NUCLEAR_REACTORCONTROLLER_H needs to not be defined yet so it is included within the appropriate context
// in ReactorMaster.h
#include "Reactor.h"

#ifndef NUCLEAR_REACTORCONTROLLER_H
#define NUCLEAR_REACTORCONTROLLER_H

#include "Internal/ChronoMaster.h"
#include "Internal/ReactorMaster.h"

namespace NUClear {
    class ReactorController : 
            public Internal::ChronoMaster<ReactorController>, 
            public Internal::ReactorMaster<ReactorController> {
        public:
            template <typename TData>
            std::shared_ptr<TData> get();
        
            template <typename TReactor>
            void install();
            
            template <typename TTrigger>
            void emit(TTrigger* data);
    };
}

template <typename TData>
std::shared_ptr<TData> NUClear::ReactorController::get() {
    return reactormaster.get<TData>();
}

template <typename TReactor>
void NUClear::ReactorController::install() {
    reactormaster.install<TReactor>();
}

template <typename TTrigger>
void NUClear::ReactorController::emit(TTrigger* data) {
    reactormaster.emit(data);
}

#endif
