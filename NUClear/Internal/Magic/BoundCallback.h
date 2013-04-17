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

#ifndef NUCLEAR_INTERNAL_MAGIC_BOUNDCALLBACK_H
#define NUCLEAR_INTERNAL_MAGIC_BOUNDCALLBACK_H

#include <tuple>
#include "Sequence.h"

namespace NUClear {
namespace Internal {
namespace Magic {
    
    // Anonymous namespace to prevent instansiation from outside this file
    namespace {
        
        /**
         * @brief Holds a bound but unexecuted function, it can be executed using its operator overload.
         * 
         * @details
         *  This class holds a function to execute, and a tuple of pointers to dereference into it. When created it will
         *  store the paramters which were passed into it in a tuple, and then when the operator is called it will
         *  dereferece these values and use them as the paramters in the function to be called.
         * 
         * @tparam TFunc    the type of function which is to be executed
         * @tparam TTypes   the object types which this callback executes with
         */
        template <typename TFunc, typename... TTypes>
        class BoundCallback {
            private:
                // The tuple which holds the callbacks
                std::tuple<TTypes...> params;
                TFunc callback;
            
                /**
                 * @brief Dereferences and uses the values from the tuple as the arguments for the function call.
                 *
                 * @details
                 *  This function uses the values which are stored in the tuple and dereferences them as parameters in
                 *  the callback function. It does this using the generated sequence of integers for this tuple. These
                 *  values are then used to extract the function parameters in order.
                 *  
                 * @param s the Sequence object which is passed in holding the int template pack
                 *
                 * @tparam S the integer pack giving the ordinal position of the tuple value to get
                 */
                template<int... S>
                void apply(Sequence<S...> s) {
                    
                    // Get each of the values from the tuple, dereference them and call the function with them
                    callback((*(std::get<S>(params)))...);
                }
            public:
                /**
                 * @brief Constructs a new BoundCallback using the passed function and arguments.
                 *
                 * @param func the callback to execute with the values
                 * @param args the arguments to call the function with
                 */
                BoundCallback(TFunc func, TTypes... args) : params(args...), callback(func) { }
            
                /**
                 * @brief Applies all of the values in the tuple to the function and executes it.
                 */
                void operator()() {
                    apply(typename GenerateSequence<sizeof...(TTypes)>::get());
                }

        };
    }
    
    /**
     * @brief Creates an object which can be executed later having the arguments bound to the passed arguments.
     *
     * @detail
     *  This function will create a new object which can be executed later. However, it has its function call arguments
     *  set as the arguments that are passed into the function. This can be used to set the arguments needed to call
     *  the function, and then pass it off to a thread pool for actual execution.
     *
     * @param function  the function to be executed
     * @param args      the arguments to bind to the function when it is run
     *
     * @tparam TFunc    the type of function that is being created
     * @tparam TTypes   the types of the function arguments that are to be bound to the function
     */
    template <typename TFunc, typename... TTypes>
    BoundCallback<TFunc, TTypes...> bindCallback(TFunc function, TTypes... args) {
        return BoundCallback<TFunc, TTypes...>(function, args...);
    }
}
}
}

#endif
