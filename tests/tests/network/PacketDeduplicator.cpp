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
#include "extension/network/PacketDeduplicator.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

namespace NUClear {
namespace extension {
    namespace network {

        SCENARIO("PacketDeduplicator basic functionality", "[network]") {
            GIVEN("a new PacketDeduplicator") {
                PacketDeduplicator dedup;

                WHEN("we check a new packet") {
                    THEN("it should not be a duplicate") {
                        REQUIRE_FALSE(dedup.is_duplicate(1));
                    }

                    AND_WHEN("we add the packet") {
                        dedup.add_packet(1);

                        THEN("it should be marked as a duplicate") {
                            REQUIRE(dedup.is_duplicate(1));
                        }

                        AND_WHEN("we check a different packet") {
                            THEN("it should not be a duplicate") {
                                REQUIRE_FALSE(dedup.is_duplicate(2));
                            }

                            AND_WHEN("we add the second packet") {
                                dedup.add_packet(2);

                                THEN("both packets should be marked as duplicates") {
                                    REQUIRE(dedup.is_duplicate(1));
                                    REQUIRE(dedup.is_duplicate(2));
                                }
                            }
                        }
                    }
                }
            }
        }

        SCENARIO("PacketDeduplicator window sliding", "[network]") {
            GIVEN("a new PacketDeduplicator") {
                PacketDeduplicator dedup;

                WHEN("we add packets up to the window size") {
                    for (uint16_t i = 0; i < 256; ++i) {
                        dedup.add_packet(i);
                    }

                    THEN("all packets should be marked as duplicates") {
                        for (uint16_t i = 0; i < 256; ++i) {
                            REQUIRE(dedup.is_duplicate(i));
                        }
                    }

                    AND_WHEN("we add a packet beyond the window") {
                        dedup.add_packet(256);

                        THEN("the oldest packet should be forgotten") {
                            REQUIRE_FALSE(dedup.is_duplicate(0));
                        }

                        AND_THEN("the newest packet should be remembered") {
                            REQUIRE(dedup.is_duplicate(256));
                        }
                    }
                }
            }
        }

        SCENARIO("PacketDeduplicator out of order packets", "[network]") {
            GIVEN("a new PacketDeduplicator") {
                PacketDeduplicator dedup;

                WHEN("we add packets out of order") {
                    dedup.add_packet(5);
                    dedup.add_packet(3);
                    dedup.add_packet(7);
                    dedup.add_packet(1);

                    THEN("all added packets should be marked as duplicates") {
                        REQUIRE(dedup.is_duplicate(1));
                        REQUIRE(dedup.is_duplicate(3));
                        REQUIRE(dedup.is_duplicate(5));
                        REQUIRE(dedup.is_duplicate(7));
                    }

                    AND_THEN("unseen packets should not be marked as duplicates") {
                        REQUIRE_FALSE(dedup.is_duplicate(2));
                        REQUIRE_FALSE(dedup.is_duplicate(4));
                        REQUIRE_FALSE(dedup.is_duplicate(6));
                        REQUIRE_FALSE(dedup.is_duplicate(8));
                    }
                }
            }
        }

        SCENARIO("PacketDeduplicator packet wrap around", "[network]") {
            GIVEN("a new PacketDeduplicator") {
                PacketDeduplicator dedup;

                WHEN("we add packets near the uint16_t wrap around point") {
                    const uint16_t start = 65530;

                    for (uint16_t i = 0; i < 10; ++i) {
                        const uint16_t packet_id = (start + i) % 65536;
                        dedup.add_packet(packet_id);
                    }

                    THEN("all added packets should be marked as duplicates") {
                        for (uint16_t i = 0; i < 10; ++i) {
                            const uint16_t packet_id = (start + i) % 65536;
                            REQUIRE(dedup.is_duplicate(packet_id));
                        }
                    }

                    AND_THEN("packets before the window should not be marked as duplicates") {
                        REQUIRE_FALSE(dedup.is_duplicate(start - 1));
                    }
                }
            }
        }

        SCENARIO("PacketDeduplicator old packets", "[network]") {
            GIVEN("a new PacketDeduplicator") {
                PacketDeduplicator dedup;

                WHEN("we add a packet and then slide the window past it") {
                    dedup.add_packet(1);

                    // Slide window past packet 1
                    for (uint16_t i = 2; i < 258; ++i) {
                        dedup.add_packet(i);
                    }

                    THEN("the old packet should not be marked as a duplicate") {
                        REQUIRE_FALSE(dedup.is_duplicate(1));
                    }

                    AND_THEN("recent packets should still be marked as duplicates") {
                        REQUIRE(dedup.is_duplicate(256));
                        REQUIRE(dedup.is_duplicate(257));
                    }
                }
            }
        }

        SCENARIO("PacketDeduplicator handles high initial packet IDs correctly", "[network]") {
            GIVEN("A PacketDeduplicator") {
                PacketDeduplicator dedup;

                WHEN("The first packet ID is greater than uint16_t max/2") {
                    const uint16_t high_id = 40000;  // > 32768
                    dedup.add_packet(high_id);

                    THEN("The first packet should be marked as a duplicate") {
                        REQUIRE(dedup.is_duplicate(high_id));
                    }
                }
            }
        }

        SCENARIO("PacketDeduplicator handles adding old packets", "[network]") {
            GIVEN("a PacketDeduplicator with a high packet ID") {
                PacketDeduplicator dedup;
                dedup.add_packet(512);  // Start with a high packet ID

                WHEN("we try to add a much older packet") {
                    dedup.add_packet(1);  // Try to add a packet that's more than 256 behind

                    THEN("the old packet should not be marked as seen") {
                        REQUIRE_FALSE(dedup.is_duplicate(1));
                    }
                }
            }
        }

    }  // namespace network
}  // namespace extension
}  // namespace NUClear
