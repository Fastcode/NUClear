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
    
    template <typename TTarget, typename... TValues>
    struct HasScope;
    
    // If we have no values then we don't have that scope
    template <typename TTarget>
    struct HasScope<TTarget> : public std::false_type {};
    
    // If we have the type (first two types are the same) then we have it
    template <typename TTarget, typename... TRest>
    struct HasScope<TTarget, TTarget, TRest...> : public std::true_type {};
    
    // If we don't have it but we have more to test, continue testing
    template <typename TFirst, typename TSecond, typename... TRest>
    struct HasScope<TFirst, TSecond, TRest...> : public HasScope<TFirst, TRest...> {};

    template <typename TReactor>
    void PowerPlant::install() {
        
        // Make sure that the class that we recieved is a reactor
        static_assert(std::is_base_of<Reactor, TReactor>::value, "You must install Reactors");
        
        // Install the reactor
        reactormaster.install<TReactor>();
    }
    
    template <typename TData>
    struct PowerPlant::Emit<Internal::CommandTypes::Scope::LOCAL, TData> {
        static void emit(PowerPlant* context, TData* data) {
            context->reactormaster.emit(data);
        }
    };

    template <typename TData>
    struct PowerPlant::Emit<Internal::CommandTypes::Scope::DIRECT, TData> {
        static void emit(PowerPlant* context, TData* data) {
            context->reactormaster.directEmit(data);
        }
    };

    template <typename... THandlers, typename TData>
    void PowerPlant::emit(std::unique_ptr<TData>&& data) {

        
        // If there are no types defined, the default is to emit local
        if(sizeof...(THandlers) == 0) {
            emit<Internal::CommandTypes::Scope::LOCAL>(std::forward<std::unique_ptr<TData>>(data));
        }
        else {
            // Release our data from the pointer
            TData* ptr = data.release();

            // For some reason GCC thinks this variable is unused? this supresses that warning
            (void) ptr;

            // TODO These functions should be noexcept
            Internal::Magic::unpack((PowerPlant::Emit<THandlers, TData>::emit(this, ptr), 0)...);
        }
    }
}
