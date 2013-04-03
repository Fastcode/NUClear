#include "Reaction.h"

namespace NUClear {

    Reaction::Reaction(std::function<void ()> callback) :
        m_callback(callback) {

    }

    Reaction::~Reaction() {}

    void Reaction::operator()() {
        m_callback();
    }

}
