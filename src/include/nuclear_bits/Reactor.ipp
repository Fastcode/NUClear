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
    
    /**
     * @brief Demangles the passed symbol to a string, or returns it if it cannot demangle it
     *
     * @param symbol the symbol to demangle
     *
     * @return the demangled symbol, or the original string if it could not be demangeld
     */
    inline std::string demangle(const char* symbol) {
        
        int status = -4; // some arbitrary value to eliminate the compiler warning
        std::unique_ptr<char, void(*)(void*)> res {
            abi::__cxa_demangle(symbol, nullptr, nullptr, &status),
            std::free
        };
        
        return std::string(status == 0 ? res.get() : symbol);
    }
    
    template <typename... TParams, typename TFunc>
    void Reactor::on(TFunc callback) {
        
        // Forward an empty string as our user reaction name
        return on<TParams...>("", std::forward<TFunc>(callback));
    }
    
    template <typename... TDSL, typename TFunc>
    void Reactor::on(const std::string& name, TFunc callback) {
        
        // There must be some parameters
        static_assert(sizeof...(TDSL) > 0, "You must have at least one paramter in an on");
        
        // Get our identifier
        std::vector<std::string> identifier;
        identifier.reserve(3);
        
        // The name provided by the user
        identifier.push_back(name);
        // The DSL that was used
        identifier.push_back(demangle(typeid(std::tuple<TDSL...>).name()));
        // The type of the function used
        identifier.push_back(demangle(typeid(TFunc).name()));
        
        // Execute our DSL
        using DSL = dsl::Parse<TDSL...>;
        
        DSL::bind();
        
        
    }
    
    template <typename... THandlers, typename TData>
    void Reactor::emit(std::unique_ptr<TData>&& data) {
        powerplant.emit<THandlers...>(std::forward<std::unique_ptr<TData>>(data));
    }
    
    template <enum LogLevel level, typename... TArgs>
    void Reactor::log(TArgs... args) {
        powerplant.log<level, TArgs...>(std::forward<TArgs>(args)...);
    }
}
