/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

// Catch2's BENCHMARK_ADVANCED was assessed for this matrix but not adopted: each cell runs a full
// multi-threaded PowerPlant lifecycle (install reactor, start, shutdown) which is an integration
// benchmark, not a micro-benchmark. Catch2's harness defaults to many warm-up/sample iterations
// per BENCHMARK registration, runs benchmarks as separate tagged cases (not one summary table),
// and cannot express template SyncMode variants cleanly alongside GENERATE without duplicating
// three near-identical TEST_CASE bodies. The hand-rolled matrix below keeps one pass per cell,
// preserves the tabular output, and stays behind the `[.benchmark]` hidden tag.

#include <algorithm>
#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "nuclear"

namespace {

    /// Total number of ping-pong hops a single chain performs before it terminates.
    constexpr int CHAIN_LENGTH = 10000;

    /// Sync mode for the benchmark reactor.
    enum class SyncMode : uint8_t {
        NONE,       ///< No Sync at all
        SINGLE,     ///< All reactions share a single Sync group
        TWO_GROUPS  ///< Reactions split between two competing Sync groups
    };

    template <SyncMode mode>
    class BenchmarkReactor : public NUClear::Reactor {
    public:
        struct SyncA {};
        struct SyncB {};

        struct MessageA {
            explicit MessageA(const int& count = 0) : count(count) {}
            int count{};
        };
        struct MessageB {
            explicit MessageB(const int& count = 0) : count(count) {}
            int count{};
        };

        BenchmarkReactor(std::unique_ptr<NUClear::Environment> environment, int fanout)
            : NUClear::Reactor(std::move(environment)), fanout(fanout) {

            switch (mode) {
                case SyncMode::NONE: {
                    on<Trigger<MessageA>>().then([this](const MessageA& m) { on_a(m); });
                    on<Trigger<MessageB>>().then([this](const MessageB& m) { on_b(m); });
                } break;
                case SyncMode::SINGLE: {
                    on<Trigger<MessageA>, Sync<SyncA>>().then([this](const MessageA& m) { on_a(m); });
                    on<Trigger<MessageB>, Sync<SyncA>>().then([this](const MessageB& m) { on_b(m); });
                } break;
                case SyncMode::TWO_GROUPS: {
                    // Each chain ping-pongs between two competing Sync groups
                    on<Trigger<MessageA>, Sync<SyncA>>().then([this](const MessageA& m) { on_a(m); });
                    on<Trigger<MessageB>, Sync<SyncB>>().then([this](const MessageB& m) { on_b(m); });
                } break;
            }

            on<Startup>().then([this] {
                for (int i = 0; i < this->fanout; ++i) {
                    emit(std::make_unique<MessageA>());
                }
            });
        }

        std::atomic<int> finished_count{0};
        int fanout{};

    private:
        void on_a(const MessageA& m) {
            if (m.count < CHAIN_LENGTH) {
                emit(std::make_unique<MessageB>(m.count + 1));
            }
            else {
                if (finished_count.fetch_add(1, std::memory_order_relaxed) + 1 == fanout) {
                    powerplant.shutdown();
                }
            }
        }

        void on_b(const MessageB& m) {
            if (m.count < CHAIN_LENGTH) {
                emit(std::make_unique<MessageA>(m.count + 1));
            }
        }
    };

    template <SyncMode mode>
    std::int64_t run_benchmark(const int pool_concurrency, const int fanout) {
        NUClear::Configuration config;
        config.default_pool_concurrency = pool_concurrency;

        NUClear::PowerPlant plant(config);
        plant.install<BenchmarkReactor<mode>>(fanout);

        const auto start = std::chrono::high_resolution_clock::now();
        plant.start();
        const auto end = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    std::string mode_name(const SyncMode m) {
        switch (m) {
            case SyncMode::NONE: return "no-sync     ";
            case SyncMode::SINGLE: return "single-sync ";
            case SyncMode::TWO_GROUPS: return "two-syncs   ";
        }
        return "?";
    }

    template <SyncMode mode>
    void run_matrix() {
        const int hw      = int(std::thread::hardware_concurrency());
        const int hw_half = std::max(1, hw / 2);

        const std::array<int, 4> concurrencies{{1, hw_half, hw, hw * 2}};
        const std::array<int, 3> fanouts{{1, hw, hw * 4}};

        std::ostringstream out;
        out << "\n=== Benchmark: " << mode_name(mode) << " (chain=" << CHAIN_LENGTH << ") ===\n";
        out << std::setw(12) << "threads" << std::setw(12) << "fanout" << std::setw(12) << "µs" << "\n";
        out << "    ----------------------------------\n";

        std::int64_t total = 0;
        for (const int concurrency : concurrencies) {
            for (const int fanout : fanouts) {
                const std::int64_t us = run_benchmark<mode>(concurrency, fanout);
                out << std::setw(12) << concurrency << std::setw(12) << fanout << std::setw(12) << us << "\n";
                total += us;
            }
        }
        out << "    total: " << total << "µs\n";

        std::cout << out.str() << std::endl;
    }

}  // namespace

// These cases are hidden (the leading '.' in the tag) so they do not run as part of the default
// CTest suite: the scheduling benchmark matrix is slow and timing-sensitive, which would slow CI
// and add flakiness. Run them explicitly with `./Benchmark "[benchmark]"` (or `[.]`) when wanted.
TEST_CASE("Benchmark emit ping-pong without sync", "[.benchmark]") {
    run_matrix<SyncMode::NONE>();
}

TEST_CASE("Benchmark emit ping-pong with a single sync", "[.benchmark]") {
    run_matrix<SyncMode::SINGLE>();
}

TEST_CASE("Benchmark emit ping-pong with two competing syncs", "[.benchmark]") {
    run_matrix<SyncMode::TWO_GROUPS>();
}
