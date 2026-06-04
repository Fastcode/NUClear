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

#ifndef NUCLEAR_NETWORK_RTT_ESTIMATOR_HPP
#define NUCLEAR_NETWORK_RTT_ESTIMATOR_HPP

#include <chrono>
#include <stdexcept>

namespace NUClear {
namespace network {

    /**
     * TCP-style Round Trip Time (RTT) estimation using Jacobson/Karels algorithm (RFC 6298).
     *
     * Uses Exponentially Weighted Moving Average (EWMA) to smooth RTT measurements and
     * calculate a retransmission timeout (RTO) value.
     */
    class RTTEstimator {
    public:
        /**
         * Construct a new RTT Estimator.
         *
         * @param alpha           Weight for RTT smoothing (default: 0.125, TCP standard)
         * @param beta            Weight for RTT variation (default: 0.25, TCP standard)
         * @param initial_rtt     Initial RTT estimate in seconds (default: 1.0)
         * @param initial_rtt_var Initial RTT variation in seconds (default: 0.0)
         * @param min_rto         Minimum RTO value in seconds (default: 0.1)
         * @param max_rto         Maximum RTO value in seconds (default: 60.0)
         */
        RTTEstimator(float alpha           = 0.125f,
                     float beta            = 0.25f,
                     float initial_rtt     = 1.0f,
                     float initial_rtt_var = 0.0f,
                     float min_rto         = 0.1f,
                     float max_rto         = 60.0f);

        /**
         * Update the RTT estimate with a new measurement.
         *
         * Applies the Jacobson/Karels algorithm:
         *   RTTVAR = (1 - beta) * RTTVAR + beta * |SRTT - sample|
         *   SRTT   = (1 - alpha) * SRTT + alpha * sample
         *   RTO    = SRTT + 4 * RTTVAR
         *
         * @param time The measured round trip time
         */
        void measure(std::chrono::steady_clock::duration time);

        /**
         * Get the current retransmission timeout.
         *
         * @return The RTO as a duration
         */
        std::chrono::steady_clock::duration timeout() const;

    private:
        float alpha;         ///< Weight for RTT smoothing
        float beta;          ///< Weight for RTT variation
        float min_rto;       ///< Minimum RTO value in seconds
        float max_rto;       ///< Maximum RTO value in seconds
        float smoothed_rtt;  ///< Smoothed RTT estimate in seconds
        float rtt_var;       ///< RTT variation in seconds
        float rto;           ///< Current retransmission timeout in seconds
        bool has_measurement{false};  ///< Whether we've received at least one measurement
    };

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_RTT_ESTIMATOR_HPP
