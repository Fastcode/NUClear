/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
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
#include "RTTEstimator.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace NUClear {
namespace extension {
    namespace network {

        RTTEstimator::RTTEstimator(float alpha,
                                   float beta,
                                   float initial_rtt,
                                   float initial_rtt_var,
                                   float min_rto,
                                   float max_rto)
            : alpha(alpha)
            , beta(beta)
            , min_rto(min_rto)
            , max_rto(max_rto)
            , smoothed_rtt(initial_rtt)
            , rtt_var(initial_rtt_var)
            , rto(std::min(std::max(initial_rtt + 4 * initial_rtt_var, min_rto), max_rto)) {

            if (alpha < 0.0f || alpha > 1.0f) {
                throw std::invalid_argument("alpha must be in range [0,1]");
            }
            if (beta < 0.0f || beta > 1.0f) {
                throw std::invalid_argument("beta must be in range [0,1]");
            }
            if (min_rto >= max_rto) {
                throw std::invalid_argument("min_rto must be less than max_rto");
            }
        }

        void RTTEstimator::measure(std::chrono::steady_clock::duration time) {
            // Convert measurement to float seconds
            const std::chrono::duration<float> m = std::chrono::duration_cast<std::chrono::duration<float>>(time);
            const float sample_rtt               = m.count();

            // Calculate RTT variation
            const float err = sample_rtt - smoothed_rtt;
            rtt_var         = (1 - beta) * rtt_var + beta * std::abs(err);

            // Update smoothed RTT
            smoothed_rtt = (1 - alpha) * smoothed_rtt + alpha * sample_rtt;

            // Calculate RTO (smoothed RTT + 4 * RTT variation) and bound to limits
            rto = std::min(std::max(smoothed_rtt + 4 * rtt_var, min_rto), max_rto);
        }

        std::chrono::steady_clock::duration RTTEstimator::timeout() const {
            return std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(rto));
        }

    }  // namespace network
}  // namespace extension
}  // namespace NUClear
