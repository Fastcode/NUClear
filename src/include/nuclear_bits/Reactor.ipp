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
    
    template <typename... TParams, typename TFunc>
    std::vector<threading::ReactionHandle> Reactor::on(TFunc callback) {
        
        // Forward an empty string as our user reaction name
        return on<TParams...>("", std::forward<TFunc>(callback));
    }
    
    template <typename... TDSL, typename TFunc>
    std::vector<threading::ReactionHandle> Reactor::on(const std::string& name, TFunc&& callback) {
        
        // There must be some parameters
        static_assert(sizeof...(TDSL) > 0, "You must have at least one paramter in an on");
        
        // Execute our compile time DSL Fusion
        using DSL = dsl::Parse<TDSL...>;
        
        auto handles = DSL::bind(std::forward<Reactor&>(*this), std::forward<const std::string&>(name), std::forward<TFunc&&>(callback));
        
        reactionHandles.insert(std::end(reactionHandles), std::begin(handles), std::end(handles));
        
        return handles;
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
