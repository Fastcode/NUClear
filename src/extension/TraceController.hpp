/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

namespace NUClear {
namespace extension {

    class TraceController : public Reactor {
    public:
        explicit TraceController(std::unique_ptr<NUClear::Environment> environment);

    private:
        struct TracePool {
            static constexpr const char* name = "Trace";
            /// Single thread to avoid multithreading concerns
            static constexpr int thread_count = 1;
            /// This pool shouldn't interfere with the system idle time
            static constexpr bool counts_for_idle = false;
            /// Make sure trace tasks execute on the trace pool even if direct emitted (e.g. logs)
            static constexpr bool tasks_must_run_on_pool = true;
            /// So we can trace events all the way to after shutdown, this pool must not shut down until destruction
            static constexpr bool continue_on_shutdown = true;
        };

        /**
         * Returns an interned id for the category name creating and writing it to the trace file if it does not exist.
         *
         * @param name The name of the category
         *
         * @return The interned id for the category
         */
        uint64_t category(const std::string& name);

        /**
         * Returns an interned id for the event name creating and writing it to the trace file if it does not exist.
         *
         * @param ids The identifiers for the event
         *
         * @return The interned id for the event
         */
        uint64_t event_name(const std::shared_ptr<const NUClear::threading::ReactionIdentifiers>& ids);

        /**
         * Returns an interned id for the source location creating and writing it to the trace file if it does not
         * exist.
         *
         * @param ids The identifiers for the source location
         *
         * @return  The interned id for the source location
         */
        uint64_t source_location(const std::shared_ptr<const NUClear::threading::ReactionIdentifiers>& ids);

        /**
         * Returns an interned id for the log message body creating and writing it to the trace file if it does not
         * exist.
         *
         * @param name The name of the log message body
         *
         * @return The interned id for the log message body
         */
        uint64_t log_message_body(const std::string& name);

        /**
         * Returns a unique id for the process track descriptor creating and writing it to the trace file if it does not
         * exist.
         *
         * @return The unique id for the process track descriptor
         */
        uint64_t process();

        /**
         * Returns a unique id for the thread track descriptor creating and writing it to the trace file if it does not
         * exist.
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

        bool process_uuid_set = false;
        std::map<std::shared_ptr<const util::ThreadPoolDescriptor>, uint64_t> pool_uuids;
        std::map<std::thread::id, uint64_t> thread_uuids;
        std::map<std::string, uint64_t, std::less<>> category_iids;
        std::map<std::shared_ptr<const NUClear::threading::ReactionIdentifiers>, uint64_t> event_name_iids;

        std::map<std::shared_ptr<const NUClear::threading::ReactionIdentifiers>, uint64_t> source_location_iids;
        std::map<std::string, uint64_t, std::less<>> log_message_body_iids;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_TRACE_CONTROLLER_HPP
