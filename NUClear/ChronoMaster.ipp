#include <algorithm>

namespace NUClear {
    template <int ticks, class period>
    void ReactorController::ChronoMaster::add() {
        // Check if we have not already loaded this type in
        if(m_loaded.find(typeid(NUClear::Internal::Every<ticks, period>)) == std::end(m_loaded)) {
            
            // Flag this type as loaded
            m_loaded.insert(typeid(NUClear::Internal::Every<ticks, period>));
            
            std::function<void ()> emit = [this](){
                m_parent->emit(new NUClear::Internal::Every<ticks, period>());
            };
            
            // Get the number of nanoseconds this tick is
            std::chrono::nanoseconds step(std::chrono::duration_cast<std::chrono::nanoseconds>(period(ticks)));
            
            // See if we already have one with this period
            auto item = std::find_if(std::begin(m_steps), std::end(m_steps), [step](std::unique_ptr<Step>& find) {
                return find->step == step;
            });
            
            // If we don't then create a new one with our initial data
            if(item == std::end(m_steps)) {
                std::unique_ptr<Step> s(new Step());
                s->step = step;
                s->callbacks.push_back(emit);
                m_steps.push_back(std::move(s));
            }
            
            // Otherwise just add the callback to the existing element
            else {
                (*item)->callbacks.push_back(emit);
            }
        }
    }
}
