#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"

using TimeUnit = test_util::TimeUnit;

constexpr int64_t EVENT_1_TIME = 4;
constexpr int64_t EVENT_2_TIME = 8;

struct Results {
    struct TimePair {
        NUClear::clock::time_point nuclear;
        std::chrono::steady_clock::time_point steady;
    };

    TimePair start;
    TimePair zero;
    std::array<TimePair, 2> events;
};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment)
        : TestBase(std::move(environment), false, std::chrono::seconds(3)) {

        on<Startup>().then([this] {
            // Reset clock to zero
            NUClear::clock::set_clock(NUClear::clock::time_point());
            results.zero = Results::TimePair{NUClear::clock::now(), std::chrono::steady_clock::now()};

            // Emit a chrono task to run at time EVENT_1_TIME
            emit<Scope::INLINE>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    results.events[0] = Results::TimePair{NUClear::clock::now(), std::chrono::steady_clock::now()};
                    return false;
                },
                NUClear::clock::time_point(TimeUnit(EVENT_1_TIME)),
                1));

            // Emit a chrono task to run at time EVENT_2_TIME, and shutdown
            emit<Scope::INLINE>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    results.events[1] = Results::TimePair{NUClear::clock::now(), std::chrono::steady_clock::now()};
                    powerplant.shutdown();
                    return false;
                },
                NUClear::clock::time_point(TimeUnit(EVENT_2_TIME)),
                2));

            // Time travel!
            emit<Scope::INLINE>(
                std::make_unique<NUClear::message::TimeTravel>(NUClear::clock::time_point(adjustment), rtf, action));

            results.start = Results::TimePair{NUClear::clock::now(), std::chrono::steady_clock::now()};
        });
    }

    // Time travel action
    NUClear::message::TimeTravel::Action action = NUClear::message::TimeTravel::Action::RELATIVE;

    // Time adjustment
    NUClear::clock::duration adjustment = std::chrono::milliseconds(0);

    // Real-time factor
    double rtf = 1.0;

    // Results struct
    Results results;
};


TEST_CASE("Test time travel correctly changes the time for non zero rtf", "[time_travel][chrono_controller]") {

    using Action = NUClear::message::TimeTravel::Action;

    const NUClear::Configuration config;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<NUClear::extension::ChronoController>();
    auto& reactor = plant.install<TestReactor>();

    // Set the reactor fields to the values we want to test
    const Action action      = GENERATE(Action::RELATIVE, Action::ABSOLUTE, Action::NEAREST);
    const int64_t adjustment = GENERATE(-4, -2, 0, 2, 4, 6, 8, 10);
    const double rtf         = GENERATE(0.5, 1.0, 2.0);
    CAPTURE(action, adjustment, rtf);
    reactor.action     = action;
    reactor.adjustment = TimeUnit(adjustment);
    reactor.rtf        = rtf;

    // Start the powerplant
    plant.start();

    // Expected results
    std::array<int64_t, 2> expected{};
    switch (action) {
        case Action::RELATIVE: expected = {EVENT_1_TIME, EVENT_2_TIME}; break;
        case Action::ABSOLUTE:
            expected = {
                std::max(int64_t(0), int64_t(EVENT_1_TIME - adjustment)),
                std::max(int64_t(0), int64_t(EVENT_2_TIME - adjustment)),
            };
            break;
        case Action::NEAREST:
            expected = adjustment < EVENT_1_TIME
                           ? std::array<int64_t, 2>{EVENT_1_TIME - adjustment, EVENT_2_TIME - adjustment}
                           : std::array<int64_t, 2>{0, EVENT_2_TIME - EVENT_1_TIME};
            break;
        default: throw std::runtime_error("Unknown action");
    }

    std::array<TimeUnit, 2> expected_nuclear = {TimeUnit(expected[0]), TimeUnit(expected[1])};
    std::array<TimeUnit, 2> expected_steady  = {TimeUnit(std::lround(double(expected[0]) / rtf)),
                                                TimeUnit(std::lround(double(expected[1]) / rtf))};

    const auto& r       = reactor.results;
    const auto& n_start = reactor.results.start.nuclear;
    const auto& s_start = reactor.results.start.steady;

    std::array<TimeUnit, 2> actual_nuclear = {
        test_util::round_to_test_units(r.events[0].nuclear - n_start),
        test_util::round_to_test_units(r.events[1].nuclear - n_start),
    };
    std::array<TimeUnit, 2> actual_steady = {
        test_util::round_to_test_units(r.events[0].steady - s_start),
        test_util::round_to_test_units(r.events[1].steady - s_start),
    };

    const TimeUnit actual_adjustment(test_util::round_to_test_units(r.start.nuclear - r.zero.nuclear));
    const TimeUnit expected_adjustment(std::min(adjustment, action == Action::NEAREST ? EVENT_1_TIME : adjustment));
    CHECK(test_util::round_to_test_units(r.zero.nuclear.time_since_epoch()) == TimeUnit(0));
    CHECK(expected_nuclear[0] == actual_nuclear[0]);
    CHECK(expected_nuclear[1] == actual_nuclear[1]);
    CHECK(expected_steady[0] == actual_steady[0]);
    CHECK(expected_steady[1] == actual_steady[1]);
    CHECK(expected_adjustment == actual_adjustment);
}
