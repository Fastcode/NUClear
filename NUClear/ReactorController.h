#ifndef NUCLEAR_REACTORCONTROLLER_H
#define NUCLEAR_REACTORCONTROLLER_H

#include "Internal/ChronoMaster.h"
#include "Internal/ReactorMaster.h"

namespace NUClear {
        
    class ReactorController : public Internal::ChronoMaster<ReactorController>, public Internal::ReactorMaster {
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
