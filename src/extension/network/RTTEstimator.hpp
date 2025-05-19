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
#ifndef NUCLEAR_EXTENSION_NETWORK_RTT_ESTIMATOR_HPP
#define NUCLEAR_EXTENSION_NETWORK_RTT_ESTIMATOR_HPP

#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace NUClear {
namespace extension {
    namespace network {

        /**
         * Implements TCP-style Round Trip Time (RTT) estimation using Jacobson/Karels algorithm.
         *
         * This class provides RTT estimation functionality similar to TCP's RTT estimation mechanism.
         * It uses an Exponentially Weighted Moving Average (EWMA) to smooth RTT measurements and
         * calculate a retransmission timeout (RTO) value. The implementation follows the TCP
         * Jacobson/Karels algorithm which provides robust RTT estimation that:
         * - Smoothly tracks the mean RTT
         * - Adapts to RTT variations
         * - Handles network jitter
         * - Provides conservative timeout values
         */
        class RTTEstimator {
        public:
            /**
             * Construct a new RTT Estimator
             *
             * @param alpha           Weight for RTT smoothing (default: 0.125, TCP standard)
             * @param beta            Weight for RTT variation (default: 0.25, TCP standard)
             * @param initial_rtt     Initial RTT estimate in seconds (default: 1.0)
             * @param initial_rtt_var Initial RTT variation in seconds (default: 0.0)
             * @param min_rto         Minimum RTO value in seconds (default: 0.1)
             * @param max_rto         Maximum RTO value in seconds (default: 60.0)
             *
             * The alpha and beta parameters control how quickly the estimator adapts to changes:
             * - alpha: Lower values (e.g. 0.125) make the smoothed RTT more stable but slower to adapt
             * - beta:  Lower values (e.g. 0.25) make the RTT variation more stable but slower to adapt
             *
             * @throws std::invalid_argument if alpha or beta are not in range [0,1]
             * @throws std::invalid_argument if min_rto >= max_rto
             */
            RTTEstimator(float alpha           = 0.125f,
                         float beta            = 0.25f,
                         float initial_rtt     = 1.0f,
                         float initial_rtt_var = 0.0f,
                         float min_rto         = 0.1f,
                         float max_rto         = 60.0f);

            /**
             * Update the RTT estimate with a new measurement
             *
             * Updates the smoothed RTT, RTT variation, and RTO using the Jacobson/Karels algorithm:
             * 1. RTT variation = (1 - beta) * old_variation + beta * |smoothed_rtt - new_rtt|
             * 2. Smoothed RTT = (1 - alpha) * old_rtt + alpha * new_rtt
             * 3. RTO = smoothed_rtt + 4 * rtt_var
             *
             * The RTO is bounded between min_rto and max_rto to prevent extreme values.
             *
             * @param time The measured round trip time
             */
            void measure(std::chrono::steady_clock::duration time);

            /**
             * Get the current retransmission timeout
             *
             * @return The RTO as a duration. This value represents the recommended timeout
             *         for network operations based on the current RTT estimates.
             */
            std::chrono::steady_clock::duration timeout() const;

        private:
            float alpha;         ///< Weight for RTT smoothing (typically 0.125)
            float beta;          ///< Weight for RTT variation (typically 0.25)
            float min_rto;       ///< Minimum RTO value in seconds
            float max_rto;       ///< Maximum RTO value in seconds
            float smoothed_rtt;  ///< Smoothed RTT estimate in seconds
            float rtt_var;       ///< RTT variation in seconds
            float rto;           ///< Retransmission timeout in seconds
        };

    }  // namespace network
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORK_RTT_ESTIMATOR_HPP
