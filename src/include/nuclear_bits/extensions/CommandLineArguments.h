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

#ifndef NUCLEAR_EXTENSIONS_COMMANDLINEARGUMENTS_H
#define NUCLEAR_EXTENSIONS_COMMANDLINEARGUMENTS_H

#include "nuclear"

namespace NUClear {
    
    /**
     * @brief This is data storage for command line arguments
     *
     * @details
     *  DSL types in general should not store data for themselves (to prevent users instansiating them)
     */
    template <>
    struct Reactor::TriggerType<dsl::CommandLineArguments> {
        typedef DataFor<dsl::CommandLineArguments, std::vector<std::string>> type;
    };
    
    /**
     * @brief Retarget gets to command line arguments to the data store
     *
     * @details
     *  As dsl types should not store data inside themselves, data is instead stored by proxy
     *  using a DataFor element. This extention point overrides such that a get obtains the correct data
     */
    template <>
    struct PowerPlant::CacheMaster::Get<dsl::CommandLineArguments> {
        static std::shared_ptr<std::vector<std::string>> get(PowerPlant& context) {
            return ValueCache<DataFor<dsl::CommandLineArguments, std::vector<std::string>>>::get()->data;
        }
    };
}

#endif
