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

#ifndef NUCLEAR_METAPROGRAMMING_SEQUENCE_H
#define NUCLEAR_METAPROGRAMMING_SEQUENCE_H

#include "MetaProgramming.h"

namespace NUClear {
namespace metaprogramming {
    
    /**
     * @brief This class is used to hold a sequence of integers as a variardic pack
     *
     * @tparam S the variardic pack containing the sequence
     *
     * @author Trent Houliston
     */
    template<int... S>
    struct Sequence { };
    
    // Anonymous
    namespace {        
        /**
         * @brief This is the recursive case for generating the sequence.
         *
         * @details
         *  This struct recursivly generates the variardic pack based on the provided parameters. It builds these until
         *  there is one consistent type.
         *
         * @tparam N the number to finish at
         * @tparam S the numbers we have generated (starts as only our last number)
         *
         * @author Trent Houliston
         */
        template<int N, int... S>
        struct GenSequence : GenSequence<N-1, N-1, S...> { };
        
        /**
         * @brief This is the final output of the recursive case, it contains the generated sequence accessable by type
         *
         * @details
         *  This is the final result of the recursive case, it's generate Sequence object which contains the variardic pack
         *  can be accessed by using GeneratedSequence<Value>::type;
         *
         * @tparam S the variardic pack of integers
         *
         * @author Trent Houliston
         */
        template<int... S>
        struct GenSequence<0, S...> {
            using type = Sequence<S...>;
        };
    }
    
    /**
     * @brief Holds a generated integer sequence of numbers as a variardic pack.
     *
     * @details
     *  This class is used to generate a variardic template pack which contains all of the integers from 0 to Count.
     *  This can then be used in other templates or tempalate expansions as needed (for instance it is useful for
     *  expanding tuples). 
     *
     * @tparam Count the number to generate to (from 0 to Count-1)
     *
     * @author Trent Houliston
     */
    template <int Count>
    using GenerateSequence = Meta::Do<GenSequence<Count>>;
}
}
#endif
