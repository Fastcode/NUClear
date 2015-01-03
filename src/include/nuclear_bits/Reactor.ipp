/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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
    
    template <typename DSL, typename... TArgs>
    struct Reactor::Binder {
    public:
        Binder(Reactor& r, TArgs&&... args)
        : reactor(r)
        , args(args...) {
        }
        
        template <typename TFunc>
        std::vector<ReactionHandle> then(const std::string& label, TFunc&& callback) {
            
            return then(label, std::forward<TFunc>(callback), util::GenerateSequence<sizeof...(TArgs)>());
            
        }
        
        template <typename TFunc>
        std::vector<ReactionHandle> then(TFunc&& callback) {
            return then("", std::forward<TFunc>(callback));
        }
        
    private:
        
        template <typename TFunc, int... Index>
        std::vector<ReactionHandle> then(const std::string& label, TFunc&& callback, const util::Sequence<Index...>&) {
            
            std::vector<ReactionHandle> handles = DSL::bind(reactor, label, std::forward<TFunc>(callback), std::get<Index>(args)...);
            
            // Put all of the handles into our global list so we can debind them on destruction
            reactor.reactionHandles.insert(std::end(reactor.reactionHandles), std::begin(handles), std::end(handles));
            
            return handles;
        }
        
        Reactor& reactor;
        std::tuple<TArgs...> args;
    };

    template <typename... TDSL, typename... TArgs>
    Reactor::Binder<dsl::Parse<TDSL...>, TArgs...> Reactor::on(TArgs&&... args) {

        // There must be some parameters
        static_assert(sizeof...(TDSL) > 0, "You must have at least one parameter in an on");
        
        return Binder<dsl::Parse<TDSL...>, TArgs...>(*this, std::forward<TArgs>(args)...);
    }

    template <typename... THandlers, typename TData>
    void Reactor::emit(std::unique_ptr<TData>&& data) {
        powerplant.emit<THandlers...>(std::forward<std::unique_ptr<TData>>(data));
    }

    template <enum LogLevel level, typename... TArgs>
    void Reactor::log(TArgs... args) {
        // If the log is above or equal to our log level
        if (level >= environment->logLevel) {
            powerplant.log<level, TArgs...>(std::forward<TArgs>(args)...);
        }
    }
}
