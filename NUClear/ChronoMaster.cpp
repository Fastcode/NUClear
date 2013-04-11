#include "ReactorController.h"
namespace NUClear {
    ReactorController::ChronoMaster::ChronoMaster(ReactorController* parent) :
        ReactorController::BaseMaster(parent),
        m_execute(true) {}

    ReactorController::ChronoMaster::~ChronoMaster() {
        m_execute = false;
    }

    void ReactorController::ChronoMaster::run() {
        // Initialize all of the m_steps with our start time
        std::chrono::nanoseconds start(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()));
        for(auto it = std::begin(m_steps); it != std::end(m_steps); ++it) {
            (*it)->next = start;
        }
        
        // Loop until it is time for us to finish
        while(m_execute) {
            // Get the current time
            std::chrono::nanoseconds now(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()));
            
            // Check if any intervals are before now and if so execute their callbacks and add their step.
            for(auto it = std::begin(m_steps); it != std::end(m_steps); ++it) {
                if(((*it)->next - now).count() <= 0) {
                    for(auto callback = std::begin((*it)->callbacks); callback != std::end((*it)->callbacks); ++callback) {
                        (*callback)();
                    }
                    (*it)->next += (*it)->step;
                }
                // Since we are sorted, we can ignore any after this time
                else {
                    break;
                }
            }
            
            // Sort the list so the next soonest interval is on top
            std::sort(std::begin(m_steps), std::end(m_steps), [](const std::unique_ptr<Step>& a, const std::unique_ptr<Step>& b) {
                return a->next < b->next;
            });
            
            // Sleep until it's time to emit this event
            std::this_thread::sleep_for(m_steps.front()->next - now);
        }
    }
}
