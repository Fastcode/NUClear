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
    
    template <enum Internal::CommandTypes::Scope TTarget, enum Internal::CommandTypes::Scope... TValues>
    struct HasScope;
    
    // If we have no values then we don't have that scope
    template <enum Internal::CommandTypes::Scope TTarget>
    struct HasScope<TTarget> : public std::false_type {};
    
    // If we have the type (first two types are the same) then we have it
    template <enum Internal::CommandTypes::Scope TTarget, enum Internal::CommandTypes::Scope... TRest>
    struct HasScope<TTarget, TTarget, TRest...> : public std::true_type {};
    
    // If we don't have it but we have more to test, continue testing
    template <enum Internal::CommandTypes::Scope TFirst, enum Internal::CommandTypes::Scope TSecond, enum Internal::CommandTypes::Scope... TRest>
    struct HasScope<TFirst, TSecond, TRest...> : public HasScope<TFirst, TRest...> {};

    template <typename TReactor>
    void PowerPlant::install() {
        reactormaster.install<TReactor>();
    }

    template <enum Internal::CommandTypes::Scope... TScopes, typename TData>
    void PowerPlant::emit(TData* data) {
        
        if(sizeof...(TScopes) != 0 && HasScope<Internal::CommandTypes::Scope::NETWORK, TScopes...>::value)
            networkmaster.emit(data);
        if(sizeof...(TScopes) == 0 || HasScope<Internal::CommandTypes::Scope::LOCAL, TScopes...>::value)
            reactormaster.emit(data);
    }
}
