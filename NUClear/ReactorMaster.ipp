/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

namespace NUClear {
    
    // === Template Implementations ===
    // As with Reactor.h this section contains all template function implementations for ReactorMaster. 
    // Unfortunately it's neccecary to have these here and outside the NUClear namespace due to our circular dependency on Reactor.
    // What this means is: This code needs to be down here, it can't be moved into the class or namespace since that'll break the 
    // forward declaration resolution. See the similar comment in Reactor.h for more information.

    template <typename TReactor>
    void ReactorController::ReactorMaster::install() {
        // The reactor constructor should handle subscribing to events
        std::unique_ptr<NUClear::Reactor> reactor(new TReactor(*m_parent));
        m_reactors.push_back(std::move(reactor));
    }

    template <typename TTrigger>
    void ReactorController::ReactorMaster::subscribe(NUClear::Reactor* reactor) {
        std::set<NUClear::Reactor*>& bindings = getReactorBindings<TTrigger>();
        bindings.insert(reactor);
    }

    template <typename TTrigger>
    std::set<NUClear::Reactor*>& ReactorController::ReactorMaster::getReactorBindings() {
        if(m_reactorBindings.find(typeid(TTrigger)) == m_reactorBindings.end()) {
            m_reactorBindings[typeid(TTrigger)] = std::set<NUClear::Reactor*>();
        }
        return m_reactorBindings[typeid(TTrigger)];
    }

    template <typename TData>
    std::shared_ptr<TData> ReactorController::ReactorMaster::get() {
        if(m_cache.find(typeid(TData)) == m_cache.end()) {
            // TODO: Better error stuff
            std::cerr << "Trying to get missing TData:" << std::endl;
        }

        return std::static_pointer_cast<TData>(m_cache[typeid(TData)]);
    }

    template <typename TTrigger>
    void ReactorController::ReactorMaster::emit(TTrigger* data) {
        cache<TTrigger>(data);
        notifyReactors<TTrigger>(); 
    }

    // == Private Methods ==
    template <typename TTrigger>
    void ReactorController::ReactorMaster::cache(TTrigger* data) {
        m_cache[typeid(TTrigger)] = std::shared_ptr<void>(data);
    }

    template <typename TTrigger>
    void ReactorController::ReactorMaster::notifyReactors() {
        std::set<NUClear::Reactor*>& reactors = getReactorBindings<TTrigger>();
        for(std::set<NUClear::Reactor*>::iterator reactor = std::begin(reactors); reactor != std::end(reactors); ++reactor) {
            (*reactor)->notify<TTrigger>();
        }
    }
}
