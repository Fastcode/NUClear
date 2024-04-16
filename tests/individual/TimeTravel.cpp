#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

using TestUnits                = std::chrono::duration<int64_t, std::ratio<1, 50>>;
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

class TestReactor : public test_util::TestBase<TestReactor, 5000> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            // Reset clock to zero
            NUClear::clock::set_clock(NUClear::clock::time_point());
            results.zero = Results::TimePair{NUClear::clock::now(), std::chrono::steady_clock::now()};

            // Emit a chrono task to run at time EVENT_1_TIME
            emit<Scope::DIRECT>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    results.events[0] = Results::TimePair{NUClear::clock::now(), std::chrono::steady_clock::now()};
                    return false;
                },
                NUClear::clock::time_point(TestUnits(EVENT_1_TIME)),
                1));

            // Emit a chrono task to run at time EVENT_2_TIME, and shutdown
            emit<Scope::DIRECT>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    results.events[1] = Results::TimePair{NUClear::clock::now(), std::chrono::steady_clock::now()};
                    powerplant.shutdown();
                    return false;
                },
                NUClear::clock::time_point(TestUnits(EVENT_2_TIME)),
                2));

            // Time travel!
            emit<Scope::DIRECT>(
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

}  // anonymous namespace

TEST_CASE("Test time travel correctly changes the time for non zero rtf", "[time_travel][chrono_controller]") {

    using Action = NUClear::message::TimeTravel::Action;

    const NUClear::Configuration config;
    auto plant    = std::make_shared<NUClear::PowerPlant>(config);
    auto& reactor = plant->install<TestReactor>();

    // Set the reactor fields to the values we want to test
    const Action action      = GENERATE(Action::RELATIVE, Action::ABSOLUTE, Action::NEAREST);
    const int64_t adjustment = GENERATE(-4, -2, 0, 2, 4, 6, 8, 10);
    const double rtf         = GENERATE(0.5, 1.0, 2.0);
    CAPTURE(action, adjustment, rtf);
    reactor.action     = action;
    reactor.adjustment = TestUnits(adjustment);
    reactor.rtf        = rtf;

    // Start the powerplant
    plant->start();

    // Expected results
    std::array<int64_t, 2> expected{};
    switch (action) {
        case Action::RELATIVE: expected = {EVENT_1_TIME, EVENT_2_TIME}; break;
        case Action::ABSOLUTE:
            expected = {std::max(0L, EVENT_1_TIME - adjustment), std::max(0L, EVENT_2_TIME - adjustment)};
            break;
        case Action::NEAREST:
            expected = adjustment < EVENT_1_TIME
                           ? std::array<int64_t, 2>{EVENT_1_TIME - adjustment, EVENT_2_TIME - adjustment}
                           : std::array<int64_t, 2>{0, EVENT_2_TIME - EVENT_1_TIME};
            break;
        default: throw std::runtime_error("Unknown action");
    }

    std::array<TestUnits, 2> expected_nuclear = {TestUnits(expected[0]), TestUnits(expected[1])};
    // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
    std::array<TestUnits, 2> expected_steady = {TestUnits(std::lround(double(expected[0]) / rtf)),
                                                TestUnits(std::lround(double(expected[1]) / rtf))};

    const auto& r       = reactor.results;
    const auto& n_start = reactor.results.start.nuclear;
    const auto& s_start = reactor.results.start.steady;

    auto round_to_test_units = [](const auto& duration) {
        const double d = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
        const double t = (TestUnits::period::den * d) / TestUnits::period::num;
        return TestUnits(std::lround(t));
    };

    std::array<TestUnits, 2> actual_nuclear = {
        round_to_test_units(r.events[0].nuclear - n_start),
        round_to_test_units(r.events[1].nuclear - n_start),
    };
    std::array<TestUnits, 2> actual_steady = {
        round_to_test_units(r.events[0].steady - s_start),
        round_to_test_units(r.events[1].steady - s_start),
    };

    const TestUnits actual_adjustment(round_to_test_units(r.start.nuclear - r.zero.nuclear));
    const TestUnits expected_adjustment(std::min(adjustment, action == Action::NEAREST ? EVENT_1_TIME : adjustment));
    CHECK(expected_nuclear[0] == actual_nuclear[0]);
    CHECK(expected_nuclear[1] == actual_nuclear[1]);
    CHECK(expected_steady[0] == actual_steady[0]);
    CHECK(expected_steady[1] == actual_steady[1]);
    CHECK(expected_adjustment == actual_adjustment);
}
