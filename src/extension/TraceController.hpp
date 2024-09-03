/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#ifndef NUCLEAR_EXTENSION_TRACE_CONTROLLER_HPP
#define NUCLEAR_EXTENSION_TRACE_CONTROLLER_HPP

#include <fstream>
#include <set>

#include "../Reactor.hpp"
#include "../message/LogMessage.hpp"
#include "trace/StringInterner.hpp"
#include "trace/protobuf.hpp"

namespace NUClear {
namespace extension {

    class TraceController : public Reactor {
    public:
        explicit TraceController(std::unique_ptr<NUClear::Environment> environment);

    private:
        struct TracePool {
            static constexpr const char* name = "Trace";
            /// Single thread to avoid multithreading concerns
            static constexpr int concurrency = 1;
            /// This pool shouldn't interfere with the system idle time
            static constexpr bool counts_for_idle = false;
            /// So we can trace events all the way to after shutdown, this pool must not shut down until destruction
            static constexpr bool persistent = true;
        };

        /**
         * Writes a trace packet to the trace file.
         */
        void write_trace_packet(const std::vector<char>& packet);

        /**
         * Returns a unique id for the process track creating and writing it to the trace file if it does not exist.
         *
         * @return The unique id for the process track
         */
        uint64_t process();

        /**
         * Returns a unique id for the thread track creating and writing it to the trace file if it does not exist.
         *
         * This also creates a counter for the thread time at the same time at the unique id + 1.
         *
         * @param info The thread info to create the track descriptor for
         *
         * @return The unique id for the thread track descriptor
         */
        uint64_t thread(const message::ReactionStatistics::Event::ThreadInfo& info);

        /**
         * Creates and writes an event to the trace file.
         *
         * @param event The event to write to the trace file
         */
        void encode_event(const message::ReactionEvent& event);

        /**
         * Encodes a log message into the trace file.
         *
         * @param log_stats The statistics for the log message reaction itself
         * @param msg       The log message to encode
         */
        void encode_log(const std::shared_ptr<const message::ReactionStatistics>& log_stats,
                        const message::LogMessage& msg);

        std::ofstream trace_file;

        std::atomic<uint64_t> next_uuid{2};  // Start at 2 as 1 is reserved for the process track descriptor

        ReactionHandle event_handle;
        ReactionHandle log_handle;

        uint64_t process_uuid = 0;
        std::map<std::shared_ptr<const util::ThreadPoolDescriptor>, uint64_t> pool_uuids;
        std::map<std::thread::id, uint64_t> thread_uuids;

        /// The interned category names
        trace::StringInterner<std::string, 1> categories;
        /// The interned event names
        trace::StringInterner<std::shared_ptr<const NUClear::threading::ReactionIdentifiers>, 2> event_names;
        /// The interned log message bodies
        trace::StringInterner<std::string, 20> log_message_bodies;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_TRACE_CONTROLLER_HPP
