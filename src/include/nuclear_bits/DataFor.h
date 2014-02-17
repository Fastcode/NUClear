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

#ifndef NUCLEAR_DATAFOR_H
#define NUCLEAR_DATAFOR_H

namespace NUClear {
    
    /**
     * @brief Holds data by proxy for datatypes that cannot be instansiated
     *
     * @details
     *  Often a type will want to be triggered on but cannot store data itself (e.g. a dsl type).
     *  For these types, this DataFor type can be created in order to hold data by proxy.
     *  This data can then be collected by redirecting get to use the DataFor type rather then
     *  the primary type.
     *
     * @tparam TFor the type to hold data for.
     * @tparam TData the data type to hold.
     *
     * @author Trent Houliston
     */
    template <typename TFor, typename TData = void>
    struct DataFor {
        DataFor() : data(std::make_shared<TData>()) {}
        DataFor(std::shared_ptr<TData> data) : data(data) {}
        std::shared_ptr<TData> data;
    };
}
#endif
