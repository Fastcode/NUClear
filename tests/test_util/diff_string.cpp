/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#include "diff_string.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

#include "lcs.hpp"

namespace test_util {

std::string diff_string(const std::vector<std::string>& expected, const std::vector<std::string>& actual) {

    // Find the longest string in each side or "Expected" and "Actual" if those are the longest
    auto len        = [](const std::string& a, const std::string& b) { return a.size() < b.size(); };
    auto max_a_it   = std::max_element(expected.begin(), expected.end(), len);
    auto max_b_it   = std::max_element(actual.begin(), actual.end(), len);
    const int max_a = std::max(int(std::strlen("Expected")), max_a_it != expected.end() ? int(max_a_it->size()) : 0);
    const int max_b = std::max(int(std::strlen("Actual")), max_b_it != actual.end() ? int(max_b_it->size()) : 0);

    // Start with a header
    std::string output = std::string("Expected") + std::string(max_a - 8, ' ') + "    |    " + std::string("Actual")
                         + std::string(max_b - 6, ' ') + "\n";

    // Print a divider characters for a divider
    output += std::string(9 + max_a + max_b, '-') + "\n";

    auto matches  = lcs(expected, actual);
    auto& match_a = matches.first;
    auto& match_b = matches.second;
    int i_a       = 0;
    int i_b       = 0;
    while (i_a < int(expected.size()) && i_b < int(actual.size())) {
        if (match_a[i_a]) {
            if (match_b[i_b]) {
                output += expected[i_a] + std::string(max_a - int(expected[i_a].size()), ' ') + "   <->   "
                          + actual[i_b] + std::string(max_b - int(actual[i_b].size()), ' ') + "\n";
                i_a++;
                i_b++;
            }
            else {
                output += std::string(max_a, ' ') + "   <->   " + actual[i_b]
                          + std::string(max_b - int(actual[i_b].size()), ' ') + "\n";
                i_b++;
            }
        }
        else {
            output += expected[i_a] + std::string(max_a - int(expected[i_a].size()), ' ') + "   <->   "
                      + std::string(max_b, ' ') + "\n";
            i_a++;
        }
    }
    while (i_a < int(expected.size())) {
        output += expected[i_a] + std::string(max_a - int(expected[i_a].size()), ' ') + "   <->   "
                  + std::string(max_b, ' ') + "\n";
        i_a++;
    }
    while (i_b < int(actual.size())) {
        output += std::string(max_a, ' ') + "   <->   " + actual[i_b]
                  + std::string(max_b - int(actual[i_b].size()), ' ') + "\n";
        i_b++;
    }

    return output;
}

}  // namespace test_util
