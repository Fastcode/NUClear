/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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
#include <utility>


namespace NUClear {

// Forward declare reactor and powerplant
class Reactor;
class PowerPlant;

/**
 * Environment defines variables that are passed from the installing PowerPlant context into a Reactor.
 *
 * The Environment is used to provide information from the PowerPlant to Reactors.
 * Each Reactor owns it's own environment and can use it to access useful information.
 */
class Environment {
public:
    Environment(PowerPlant& powerplant, std::string reactor_name)
        : powerplant(powerplant), reactor_name(std::move(reactor_name)) {}

private:
    friend class PowerPlant;
    friend class Reactor;

    /// The PowerPlant to use in this reactor
    PowerPlant& powerplant;
    /// The name of the reactor
    std::string reactor_name;
};

}  // namespace NUClear

#endif  // NUCLEAR_ENVIRONMENT_HPP
