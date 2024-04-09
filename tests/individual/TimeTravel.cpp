#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

// Helper struct for testing chrono tasks
struct ChronoTaskTest {
    NUClear::clock::time_point time        = NUClear::clock::time_point();
    bool ran                               = false;
    std::chrono::milliseconds steady_delta = std::chrono::milliseconds(0);
    std::chrono::milliseconds system_delta = std::chrono::milliseconds(0);
    NUClear::id_t id                       = 0;
    ChronoTaskTest()                       = default;
    ChronoTaskTest(NUClear::clock::time_point time_, bool ran_, NUClear::id_t id_) : time(time_), ran(ran_), id(id_) {}
};

// Struct to hold test results
struct TestResults {
    bool task_ran                          = false;
    std::chrono::milliseconds steady_delta = std::chrono::milliseconds(0);
    std::chrono::milliseconds system_delta = std::chrono::milliseconds(0);
};

class TestReactor : public test_util::TestBase<TestReactor, 5000> {
public:
    std::unique_ptr<ChronoTaskTest> test_task               = nullptr;
    NUClear::clock::duration time_travel_adjustment         = std::chrono::milliseconds(0);
    NUClear::message::TimeTravel::Action time_travel_action = NUClear::message::TimeTravel::Action::RELATIVE;
    std::chrono::milliseconds chrono_task_delay             = std::chrono::milliseconds(0);
    std::chrono::steady_clock::time_point steady_start_time = std::chrono::steady_clock::now();
    NUClear::clock::time_point system_start_time            = NUClear::clock::now();

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            steady_start_time = std::chrono::steady_clock::now();
            system_start_time = NUClear::clock::now();

            test_task        = std::make_unique<ChronoTaskTest>(NUClear::clock::now() + chrono_task_delay, false, 1);
            auto chrono_task = std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    test_task->ran       = true;
                    auto steady_end_time = std::chrono::steady_clock::now();
                    test_task->steady_delta =
                        std::chrono::duration_cast<std::chrono::milliseconds>(steady_end_time - steady_start_time);
                    auto system_end_time = NUClear::clock::now();
                    test_task->system_delta =
                        std::chrono::duration_cast<std::chrono::milliseconds>(system_end_time - system_start_time);
                    powerplant.shutdown();
                    return false;
                },
                test_task->time,
                test_task->id);
            emit<Scope::DIRECT>(std::move(chrono_task));

            emit<Scope::DIRECT>(
                std::make_unique<NUClear::message::TimeTravel>(time_travel_adjustment, 1.0, time_travel_action));
        });
    }
};

TestResults perform_test(NUClear::message::TimeTravel::Action action,
                         const NUClear::clock::duration& adjustment,
                         const std::chrono::milliseconds& shutdown_delay,
                         const std::chrono::milliseconds& task_delay) {
    const NUClear::Configuration config;
    auto plant    = std::make_shared<NUClear::PowerPlant>(config);
    auto& reactor = plant->install<TestReactor>();

    reactor.time_travel_action     = action;
    reactor.time_travel_adjustment = adjustment;
    reactor.chrono_task_delay      = task_delay;

    NUClear::clock::set_clock(NUClear::clock::time_point());

    std::thread shutdown_thread([shutdown_delay, plant]() {
        std::this_thread::sleep_for(shutdown_delay);
        plant->shutdown();
    });

    plant->start();
    shutdown_thread.join();

    // After shutdown, collect results from the reactor
    TestResults results;
    if (reactor.test_task) {
        results.task_ran     = reactor.test_task->ran;
        results.steady_delta = reactor.test_task->steady_delta;
        results.system_delta = reactor.test_task->system_delta;
    }
    return results;
}

}  // anonymous namespace

TEST_CASE("TimeTravel Actions Test", "[time_travel][chrono_controller]") {
    // Tolerance (milliseconds)
    const int tolerance = 1;

    SECTION("Action::RELATIVE with shutdown before task time") {
        auto results = perform_test(NUClear::message::TimeTravel::Action::RELATIVE,
                                    std::chrono::milliseconds(20),   // Time travel adjustment
                                    std::chrono::milliseconds(10),   // Steady clock shutdown delay
                                    std::chrono::milliseconds(20));  // Chrono task delay
        REQUIRE_FALSE(results.task_ran);
    }

    SECTION("Action::RELATIVE with shutdown after task time") {
        auto results = perform_test(NUClear::message::TimeTravel::Action::RELATIVE,
                                    std::chrono::milliseconds(20),   // Time travel adjustment
                                    std::chrono::milliseconds(30),   // Steady clock shutdown delay
                                    std::chrono::milliseconds(10));  // Chrono task delay
        REQUIRE(results.task_ran);
        REQUIRE(std::abs(results.steady_delta.count() - 10) <= tolerance);
        REQUIRE(std::abs(results.system_delta.count() - 30) <= tolerance);
    }

    SECTION("Action::ABSOLUTE with task time before adjustment time") {
        auto results = perform_test(NUClear::message::TimeTravel::Action::ABSOLUTE,
                                    std::chrono::milliseconds(20),   // Time travel adjustment
                                    std::chrono::milliseconds(30),   // Steady clock shutdown delay
                                    std::chrono::milliseconds(10));  // Chrono task delay
        REQUIRE(results.task_ran);
        REQUIRE(std::abs(results.steady_delta.count() - 0) <= tolerance);
        REQUIRE(std::abs(results.system_delta.count() - 20) <= tolerance);
    }

    SECTION("Action::ABSOLUTE with task time after adjustment time") {
        auto results = perform_test(NUClear::message::TimeTravel::Action::ABSOLUTE,
                                    std::chrono::milliseconds(20),   // Time travel adjustment
                                    std::chrono::milliseconds(10),   // Steady clock shutdown delay
                                    std::chrono::milliseconds(40));  // Chrono task delay
        REQUIRE_FALSE(results.task_ran);
    }

    SECTION("Action::NEAREST with task time before adjustment time") {
        auto results = perform_test(NUClear::message::TimeTravel::Action::NEAREST,
                                    std::chrono::milliseconds(20),   // Time travel adjustment
                                    std::chrono::milliseconds(30),   // Steady clock shutdown delay
                                    std::chrono::milliseconds(10));  // Chrono task delay
        REQUIRE(results.task_ran);
        REQUIRE(std::abs(results.steady_delta.count() - 0) <= tolerance);
        REQUIRE(std::abs(results.system_delta.count() - 10) <= tolerance);
    }
}
