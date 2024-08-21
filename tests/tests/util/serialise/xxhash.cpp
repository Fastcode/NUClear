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

#include "util/serialise/xxhash.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdint>
#include <string>
#include <utility>

// The seed used for the tests (the same one that NUClear uses)
constexpr uint32_t fixed_seed = 0x4e55436c;

SCENARIO("xxhash32 produces correct output for different input values", "[util][serialise][xxhash][xxhash32]") {

    struct TestData {
        TestData(std::string description, std::string input, uint32_t expected_seedless, uint32_t expected_with_seed)
            : description(std::move(description))
            , input(std::move(input))
            , expected_seedless(expected_seedless)
            , expected_with_seed(expected_with_seed) {}
        std::string description;
        std::string input;
        uint32_t expected_seedless;
        uint32_t expected_with_seed;
    };

    auto scenario =
        GENERATE(TestData{"0 ", "", 0x02cc5d05U, 0xb2ad21cbU},                                             // Empty
                 TestData{"1 ", "Dog", 0x6f4b8144U, 0x56227ee2U},                                          // 1
                 TestData{"4 ", "Moon", 0x17a8d2d1U, 0x60b2a5e1U},                                         // 4
                 TestData{"4,1 ", "Rocket", 0x25f912cfU, 0x30cb7dbdU},                                     // 4, 1
                 TestData{"16 ", "WonderfulJourney", 0xd742480fU, 0x9d38385aU},                            // 16
                 TestData{"16,1 ", "FantasticAdventure", 0x55c3dbe1U, 0x71ae229dU},                        // 16, 1
                 TestData{"16,4", "FuturisticTechnology", 0xdcce477eU, 0x09294bc8U},                       // 16, 4
                 TestData{"16,4,1 ", "ExplorationAndDiscovery", 0x62b24db4U, 0x1894cb9eU},                 // 16, 4, 1
                 TestData{"16,16", "ExtraterrestrialAdventureAwaits!", 0x780a85ffU, 0x0ba882d4U},          // 16x2
                 TestData{"16,16,1 ", "WeAreGoingToHaveTheMostAmazingTime!", 0xd6115330U, 0x0fd74517U},    // 16x2, 1
                 TestData{"16,16,4", "FlyingThroughSpaceWithADogOnARocket!", 0x266e60b1U, 0x9efe099cU},    // 16x2, 4
                 TestData{"16,16,4,1", "WaitTheDogJustThrewUpAllOverTheRocket", 0xff8253bcU, 0x3ba3d571U}  // 16x2, 4, 1
        );

    GIVEN("An input string from the test set") {
        INFO("Scenario: " << scenario.description);
        const std::string& input           = scenario.input;
        const uint32_t& expected_seedless  = scenario.expected_seedless;
        const uint32_t& expected_with_seed = scenario.expected_with_seed;

        WHEN("xxhash32 is called with the input and no seed") {
            const uint32_t result = NUClear::util::serialise::xxhash32(input.data(), input.size());
            THEN("the result matches the expected output") {
                REQUIRE(result == expected_seedless);
            }
        }

        WHEN("xxhash32 is called with the input and seed") {
            const uint32_t result = NUClear::util::serialise::xxhash32(input.data(), input.size(), fixed_seed);
            THEN("the result matches the expected output") {
                REQUIRE(result == expected_with_seed);
            }
        }
    }
}

SCENARIO("xxhash64 produces correct output for different input values", "[util][serialise][xxhash][xxhash64]") {

    struct TestData {
        TestData(std::string description, std::string input, uint64_t expected_seedless, uint64_t expected_with_seed)
            : description(std::move(description))
            , input(std::move(input))
            , expected_seedless(expected_seedless)
            , expected_with_seed(expected_with_seed) {}
        std::string description;
        std::string input;
        uint64_t expected_seedless;
        uint64_t expected_with_seed;
    };

    auto scenario = GENERATE(
        TestData{"0", "", 0xef46db3751d8e999ULL, 0x7f61c0c4ba912ff1ULL},                                   // Empty
        TestData{"1 ", "Plz", 0x859b8032031f8c9cULL, 0x76e6ad85dc6e4583ULL},                               // 1
        TestData{"4", "Dont", 0x20f3681fe9811012ULL, 0xf5a2cd74560db5e3ULL},                               // 4
        TestData{"4,1 ", "Judge", 0x3ca8c47c2b26906aULL, 0x15d6a92f71e0e057ULL},                           // 4, 1
        TestData{"8", "MyReally", 0xc9c81e47e0a6695ULL, 0xf13e33473ad26fabULL},                            // 8
        TestData{"8,1", "BadStrings", 0xf0f44f5c592a5264ULL, 0x84033daf4bc5ed2cULL},                       // 8, 1
        TestData{"8,4", "ThatWereMade", 0x95a99608de9627f5ULL, 0xf713fa3dcd43d4edULL},                     // 8, 4
        TestData{"8,4,1 ", "ForTheseTests", 0x9b4786c0a12edf45ULL, 0xeb7d4fee1aad176fULL},                 // 8, 4, 1
        TestData{"32", "ItIsHardToGetANumberOfCharacters", 0xf785c0d96ea87492ULL, 0xba4b98790ee5652dULL},  // 32
        TestData{"32,1 ", "EspeciallyWhenYouHavePreciseRanges", 0x2a2b226d5e759352ULL, 0x478fcf3ff4fecd9aULL},  // 32, 1
        TestData{"32,4", "ThatYouNeedToHitToMakeSureYouTestAll", 0x69027933c9c8bebULL, 0xffea1c7beb47f768ULL},  // 32, 4
        TestData{"32,8",
                 "BranchesToCoverTheAlgorithmsExhaustively",
                 0xdc19c26f1ef2dd4cULL,
                 0x9c3cb4830d25d0cdULL},  // 32, 8
        TestData{"32,4,1",
                 "EventuallyIfYouWriteEnoughGarbageHere",
                 0xd9caf6e23e7ab015ULL,
                 0x2abd54db2d3704e0ULL},  // 32, 4, 1
        TestData{"32,8,1",
                 "YouWillManageToChooseSomeWordsThatCombined",
                 0x8e0db810993909f3ULL,
                 0x625d130f60c0d929ULL},  // 32, 8, 1
        TestData{"32,8,4",
                 "SatisfyTheRequirementsThatWereSetToBeCovered",
                 0xb19b5d48d245f62eULL,
                 0xa8e9d27177d450bbULL},  // 32, 8, 4
        TestData{"32,8,4,1",
                 "IfAllElseFailsYouCanAlwaysJustFillTheEndsOfThe",
                 0xd13d0f64d7d79c28ULL,
                 0xb1f3356e7f1e2b1aULL},  // 32, 8, 4, 1
        TestData{"32,32",
                 "StringWithTextThatYouRepeatAgainAndAgainAndAgainAndAgainAndAgain",
                 0xb79d5ac6a47ead84ULL,
                 0xfaab12fe20384b44ULL},  // 32x2
        TestData{"32,32,1",
                 "AndAgainAndAgainAndAgainAndAgainAndAgainAndAgainAndAgainAndAgainAnd",
                 0xbacf19be65e7e82bULL,
                 0x18541fbdce73b199ULL},  // 32x2, 1
        TestData{"32,32,4",
                 "EventuallyThoughYouGoALittleCrazyAndJustStartPuttingRandomWordsInPea",
                 0x24866ce65b924db6ULL,
                 0x804a92087dc9ae6fULL},  // 32x2, 4
        TestData{"32,32,4,1",
                 "BananaStrawberryBlueberryAppleSeeThoseAreAllJustFruitNamesThatAreHere",
                 0xaa82d69819ec69e5ULL,
                 0x495cb40415382332ULL},  // 32x2, 4, 1
        TestData{"32,32,8",
                 "AndNowIHaveToMakeAStringThatIsSeventyTwoCharactersLongByJustMakingUpText",
                 0xf074a282d51d0656ULL,
                 0xfde3aefa5d073c20ULL},  // 32x2, 8
        TestData{"32,32,8,1",
                 "AtLeastImAlmostAtTheEndNowAlthoughTheLastFewExamplesAreSoLongItsRidiculous",
                 0x11a6a73447d8baedULL,
                 0x19969c2ad532aecfULL},  // 32x2, 8, 1
        TestData{"32,32,8,4",
                 "ThisStringNeedsToBeSeventySixCharactersLongWhichIsSixtyFourPlusEightPlusFour",
                 0x667d2bed5e6ac2efULL,
                 0x9ddcc65d11683736ULL},  // 32x2, 8, 4
        TestData{"32,32,8,4,1",
                 "AndFinallyThisLastStringNeedsToBeBetweenSeventySevenAndSeventyNineToTestTheCode",
                 0x7a044ea72453baa9ULL,
                 0x26c0edaa394162b7ULL}  // 32x2, 8, 4, 1
    );

    GIVEN("An input string from the test set") {
        INFO("Scenario: " << scenario.description);
        const std::string& input           = scenario.input;
        const uint64_t& expected_seedless  = scenario.expected_seedless;
        const uint64_t& expected_with_seed = scenario.expected_with_seed;

        WHEN("xxhash64 is called with the input and no seed") {
            const uint64_t result = NUClear::util::serialise::xxhash64(input.data(), input.size());
            THEN("the result matches the expected output") {
                REQUIRE(result == expected_seedless);
            }
        }

        WHEN("xxhash64 is called with the input and seed") {
            const uint64_t result = NUClear::util::serialise::xxhash64(input.data(), input.size(), fixed_seed);
            THEN("the result matches the expected output") {
                REQUIRE(result == expected_with_seed);
            }
        }
    }
}
