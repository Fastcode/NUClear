#include "Reaction.h"
#include <iostream>
namespace NUClear {
namespace Internal {

    Reaction::Reaction(std::function<void ()> callback) :
        m_callback(callback) {

    }

    Reaction::~Reaction() {}

    void Reaction::operator()() {
        
        const auto& start = std::chrono::steady_clock::now();
        m_callback();
        const auto& end = std::chrono::steady_clock::now();
        
        const std::chrono::nanoseconds& duration = end.time_since_epoch() - start.time_since_epoch();
        
        std::cout << duration.count() << std::endl;
    }

}
}
