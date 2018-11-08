/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#ifndef NUCLEAR_ENVIRONMENT_HPP
#define NUCLEAR_ENVIRONMENT_HPP

#include <string>

#include "LogLevel.hpp"

namespace NUClear {

// Forward declare reactor and powerplant
class Reactor;
class PowerPlant;

/**
 * @brief
 *  Environment defines variables that are passed from the installing PowerPlant context
 *  into a Reactor.
 *
 * @details
 *  The Environment is used to provide information from the PowerPlant to Reactors.
 *  Each Reactor owns it's own environment and can use it to access useful information.
 */
class Environment {
public:
    Environment(PowerPlant& powerplant, std::string&& reactor_name, LogLevel log_level)
        : powerplant(powerplant), log_level(log_level), reactor_name(reactor_name) {}

private:
    friend class PowerPlant;
    friend class Reactor;

    /// @brief The PowerPlant to use in this reactor
    PowerPlant& powerplant;
    /// @brief The log level for this reactor
    LogLevel log_level;
    /// @brief The name of the reactor
    std::string reactor_name;
};

}  // namespace NUClear

#endif  // NUCLEAR_ENVIRONMENT_HPP
