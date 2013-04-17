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

#ifndef NUCLEAR_INTERNAL_MAGIC_SEQUENCE_H
#define NUCLEAR_INTERNAL_MAGIC_SEQUENCE_H

namespace NUClear {
namespace Internal {
namespace Magic {
    
    /**
     * @brief Holds a generated integer sequence of numbers as a variardic pack.
     *
     * @details
     *  This class is used to generate a variardic template pack which contains all of the integers in the range
     *  specified. This can then be used in other templates or tempalate expansions as needed (for instance it is useful
     *  for expanding tuples). It is used by using the GenerateSequence magic class, it can either be used to generate a
     *  sequence from 0 to a number, or any range of integers e.g. @code GenerateSequence<10>::get() @endcode or @code
     *  GenerateSequence<5,15>::get() @endcode These will return a templated class of type Sequence, with the parameter
     *  pack containing the first number up to (but not including) the last number.
     *
     * @tparam S the variardic pack containing the sequence
     */
    template<int... S>
    struct Sequence { };
    
    /**
     * @brief This is the recursive case for generating the sequence.
     *
     * @details
     *  This struct recursivly generates the variardic pack based on the provided parameters. It builds these until
     *  there is one consistent type.
     *
     * @tparam N the number to start from
     * @tparam S the numbers we have generated (starts as only our last number)
     */
    template<int N, int... S>
    struct GenerateSequence : GenerateSequence<N-1, N-1, S...> { };
    
    /**
     * @brief This is the final output of the recursive case, it contains the generated sequence accessable by get()
     *
     * @details
     *  This is the final result of the recursive case, it's generate Sequence object which contains the variardic pack
     *  can be accessed by using GeneratedSequence<Value>::get();
     *
     * @tparam S the variardic pack of integers
     */
    template<int... S>
    struct GenerateSequence<0, S...> {
        typedef Sequence<S...> get;
    };
}
}
}
#endif
