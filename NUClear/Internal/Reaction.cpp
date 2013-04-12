#include "Reaction.h"
#include <iostream>
namespace NUClear {
namespace Internal {
    
    Reaction::Reaction(std::function<std::function<void ()> ()> callback, ReactionOptions options) :
    m_options(options),
    m_callback(callback)
    {
        
    }
    
    Reaction::~Reaction() {}
    
    std::unique_ptr<ReactionTask> Reaction::getTask() {
        return std::unique_ptr<ReactionTask>(new ReactionTask(*this, m_callback()));
    }
    
    ReactionTask::ReactionTask(Reaction& parent, std::function<void ()> callback) :
    m_parent(parent),
    m_callback(callback),
    m_options(parent.m_options) {
        
    }
    
    void ReactionTask::operator()() {
        
        const auto& start = std::chrono::steady_clock::now();
        m_callback();
        const auto& end = std::chrono::steady_clock::now();
        m_runtime = end.time_since_epoch() - start.time_since_epoch();
        
        std::cout << m_runtime.count() << std::endl;
    }
}
}
