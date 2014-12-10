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
#include <iostream>
#include <tuple>

#include <cxxabi.h>

#include "dsl/word/Trigger.h"
#include "dsl/word/With.h"
#include "dsl/word/Single.h"

#include "dsl/operation/CacheGet.h"
#include "dsl/operation/TypeBind.h"

#include "dsl/Parse.h"

#include "../include/nuclear_bits/metaprogramming/apply.h"

using NUClear::dsl::ParseDSL;

using namespace NUClear::dsl::word;

template <typename TType>
const std::string demangled() {

    int status = -4; // some arbitrary value to eliminate the compiler warning
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(typeid(TType).name(), nullptr, nullptr, &status),
        std::free
    };

    return std::string(status == 0 ? res.get() : typeid(TType).name());
}

struct Track {
    static int s;
    int i;
    Track() : i(++s) {
        std::cout << "Constructing default " << i << std::endl;
    }

    Track(int i) : i(i) {
        std::cout << "Constructing with " << i << std::endl;
    }

    ~Track() {
        std::cout << "Destructing " << i << std::endl;
    }

    Track(const Track& c) {

        std::cout << "Copying from " << c.i << " to " << i << std::endl;
        i = c.i;
    }

    Track(Track&& c) {
        std::cout << "Moving from " << c.i << " to " << i << std::endl;
        i = std::move(c.i);
    }
};

int Track::s = 0;

std::ostream& operator<<(std::ostream& out, const Track& t) {

    out << t.i;

    return out;
}

struct DSL {

    DSL(void (*bind)(), bool (*precondition)(), void (*postcondition)())
     : bind(bind)
     , precondition(precondition)
     , postcondition(postcondition) {
     }

    void (*bind)();
    bool (*precondition)();
    void (*postcondition)();
};

int main() {

    std::cout << "Beginning Test" << std::endl;

    using Parsed = ParseDSL<Trigger<Track>, With<Track>, Single>;

    // This gives us a get function
    auto get = Parsed::get;
    auto bind = Parsed::bind;
    auto precondition = Parsed::precondition;
    auto postcondition = Parsed::postcondition;

    std::cout << "Types of the generated functions" << std::endl;
    std::cout << demangled<decltype(get)>() << std::endl;
    std::cout << demangled<decltype(bind)>() << std::endl;
    std::cout << demangled<decltype(precondition)>() << std::endl;
    std::cout << demangled<decltype(postcondition)>() << std::endl;

    std::cout << "Creating the DSL object" << std::endl;
    DSL dsl(bind, precondition, postcondition);

    // Making an executor
    auto func = [](const Track& t1, const Track& t2) {
        std::cout << "T1: " << t1 << " T2: " << t2 << std::endl;
    };
    NUClear::metaprogramming::apply(func, get());

    std::cout << "Running bind on the DSL object" << std::endl;
    dsl.bind();

    std::cout << "Getting the precondition" << std::endl;
    std::cout << dsl.precondition() << std::endl;

    auto tup = get();

    ParseDSL<Trigger<Track>, With<Track>, Single>::bind();

    std::cout << std::get<0>(tup) << std::endl;

    std::cout << "Ending Test" << std::endl;
}