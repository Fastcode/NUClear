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

using NUClear::dsl::ParseDSL;

using namespace NUClear::dsl::word;

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



int main() {

    std::cout << "Beginning Test" << std::endl;

    auto tup = ParseDSL<Trigger<Track>, With<Track>, Single>::get();

    ParseDSL<Trigger<Track>, With<Track>, Single>::bind();

    std::cout << std::get<0>(tup) << std::endl;

    std::cout << "Ending Test" << std::endl;
}