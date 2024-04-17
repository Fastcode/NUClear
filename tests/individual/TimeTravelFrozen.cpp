#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <future>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

constexpr std::chrono::milliseconds EVENT_1_TIME  = std::chrono::milliseconds(4);
constexpr std::chrono::milliseconds EVENT_2_TIME  = std::chrono::milliseconds(8);
constexpr std::chrono::milliseconds SHUTDOWN_TIME = std::chrono::milliseconds(12);

struct WaitForShutdown {};

class TestReactor : public test_util::TestBase<TestReactor, 5000> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            // Reset clock to zero
            NUClear::clock::set_clock(NUClear::clock::time_point(), 0.0);

            // Emit a chrono task to run at time EVENT_1_TIME
            emit<Scope::DIRECT>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    events.push_back("Event 1");
                    return false;
                },
                NUClear::clock::time_point(EVENT_1_TIME),
                1));

            // Emit a chrono task to run at time EVENT_2_TIME
            emit<Scope::DIRECT>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    events.push_back("Event 2");
                    return false;
                },
                NUClear::clock::time_point(EVENT_2_TIME),
                2));

            // Time travel
            emit<Scope::DIRECT>(
                std::make_unique<NUClear::message::TimeTravel>(NUClear::clock::time_point(adjustment), rtf, action));

            // Shutdown after steady clock amount of time
            emit(std::make_unique<WaitForShutdown>());
        });

        on<Trigger<WaitForShutdown>>().then([this] {
            std::this_thread::sleep_for(SHUTDOWN_TIME);
            events.push_back("Finished");
            powerplant.shutdown();
        });
    }

    // Time travel action
    NUClear::message::TimeTravel::Action action = NUClear::message::TimeTravel::Action::RELATIVE_;

    // Time adjustment
    NUClear::clock::duration adjustment = std::chrono::milliseconds(0);

    // Real-time factor
    double rtf = 1.0;

    // Events
    std::vector<std::string> events = {};
};

}  // anonymous namespace

TEST_CASE("Test time travel correctly changes the time for non zero rtf", "[time_travel][chrono_controller]") {

    using Action = NUClear::message::TimeTravel::Action;

    const NUClear::Configuration config;
    auto plant    = std::make_shared<NUClear::PowerPlant>(config);
    auto& reactor = plant->install<TestReactor>();

    // Set the reactor fields to the values we want to test
    const Action action      = GENERATE(Action::RELATIVE_, Action::ABSOLUTE_, Action::NEAREST_);
    const int64_t adjustment = GENERATE(-4, -2, 0, 2, 4, 6, 8, 10);
    CAPTURE(action, adjustment);
    reactor.action     = action;
    reactor.adjustment = std::chrono::milliseconds(adjustment);
    reactor.rtf        = 0.0;

    // Start the powerplant
    plant->start();

    // Expected results
    std::vector<std::string> expected;
    switch (action) {
        case Action::RELATIVE_: expected = {"Finished"}; break;
        case Action::ABSOLUTE_:
            if (std::chrono::milliseconds(adjustment) < EVENT_1_TIME) {
                expected = {"Finished"};
            }
            else if (std::chrono::milliseconds(adjustment) < EVENT_2_TIME) {
                expected = {"Event 1", "Finished"};
            }
            else {
                expected = {"Event 1", "Event 2", "Finished"};
            }
            break;
        case Action::NEAREST_:
            expected = std::chrono::milliseconds(adjustment) < EVENT_1_TIME
                           ? std::vector<std::string>{"Finished"}
                           : std::vector<std::string>{"Event 1", "Finished"};
            break;
        default: throw std::runtime_error("Unknown action");
    }

    INFO(test_util::diff_string(expected, reactor.events));
    CHECK(expected == reactor.events);
}
