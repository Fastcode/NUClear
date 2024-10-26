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

#include "TraceController.hpp"

#include <chrono>
#include <regex>

#include "../message/LogMessage.hpp"
#include "../message/ReactionStatistics.hpp"
#include "../message/Trace.hpp"
#include "trace/protobuf.hpp"

namespace NUClear {
namespace extension {

    using message::BeginTrace;
    using message::EndTrace;
    using message::LogMessage;
    using message::ReactionEvent;
    using message::ReactionStatistics;

    /// Use a constant sequence id as there will only be one writer, 0 is invalid so 1 is the first valid id
    constexpr uint32_t trusted_packet_sequence_id = 1;

    // Extracted from the chromium trace format
    enum SequenceFlags : int32_t { SEQ_NEEDS_INCREMENTAL_STATE = 2, SEQ_INCREMENTAL_STATE_CLEARED = 1 };
    enum TrackDescriptorType : int32_t { TYPE_SLICE_BEGIN = 1, TYPE_SLICE_END = 2, TYPE_INSTANT = 3 };
    enum BuiltinCounterType : int32_t { COUNTER_THREAD_TIME_NS = 1 };
    enum LogMessagePriority : int32_t {
        PRIO_UNSPECIFIED = 0,
        PRIO_UNUSED      = 1,
        PRIO_VERBOSE     = 2,
        PRIO_DEBUG       = 3,
        PRIO_INFO        = 4,
        PRIO_WARN        = 5,
        PRIO_ERROR       = 6,
        PRIO_FATAL       = 7,
    };

    template <typename T>
    std::chrono::nanoseconds ts(const T& timestamp) {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch());
    }

    void TraceController::write_trace_packet(const std::vector<char>& packet) {
        // Write the packet to the file
        trace_file.write(packet.data(), ssize_t(packet.size()));
    }

    std::string name_for_id(const std::shared_ptr<const NUClear::threading::ReactionIdentifiers>& ids) {
        if (ids == nullptr) {
            // With no identifiers we can't do anything
            return "";
        }
        if (ids->name.empty()) {
            // Remove the namespace from the DSL with a regex
            return std::regex_replace(ids->dsl, std::regex("[A-Za-z_][A-Za-z0-9_]+::"), "");
        }
        return ids->name;
    }

    uint64_t TraceController::process() {
        if (process_uuid == 0) {
            process_uuid = 1;
            std::vector<char> data;
            {
                const trace::protobuf::SubMessage packet(1, data);  // packet:1
                {
                    const trace::protobuf::SubMessage track_descriptor(60, data);  // track_descriptor:60
                    trace::protobuf::uint64(1, 1, data);                           // uuid:1:uint64
                    {
                        const trace::protobuf::SubMessage process(3, data);  // process:3
                        trace::protobuf::int32(1, 1, data);                  // pid:1:int32
                        trace::protobuf::string(6, "NUClear", data);         // name:6:string
                    }
                }
            }
            write_trace_packet(data);
        }

        return process_uuid;
    }

    uint64_t TraceController::thread(const ReactionStatistics::Event::ThreadInfo& info) {
        if (thread_uuids.find(info.thread_id) != thread_uuids.end()) {
            return thread_uuids.at(info.thread_id);
        }

        const uint64_t parent_uuid = process();
        const uint64_t uuid =
            thread_uuids.emplace(info.thread_id, next_uuid.fetch_add(2, std::memory_order_relaxed)).first->second;
        const std::string name = info.pool == nullptr ? "Non NUClear" : info.pool->name;

        std::vector<char> data;
        {
            const trace::protobuf::SubMessage packet(1, data);  // packet:1
            {
                const trace::protobuf::SubMessage track_descriptor(60, data);  // track_descriptor:60
                trace::protobuf::uint64(1, uuid, data);                        // uuid:1:uint64
                trace::protobuf::uint64(5, parent_uuid, data);                 // parent_uuid:5:uint64
                {
                    const trace::protobuf::SubMessage thread(4, data);      // thread:4
                    trace::protobuf::int32(1, int32_t(parent_uuid), data);  // pid:1:int32
                    trace::protobuf::int32(2, int32_t(uuid), data);         // tid:2:int32
                    trace::protobuf::string(5, name, data);                 // name:5:string
                }
            }
        }
        {
            const trace::protobuf::SubMessage packet(1, data);  // packet:1
            {
                const trace::protobuf::SubMessage track_descriptor(60, data);  // track_descriptor:60
                trace::protobuf::uint64(1, uuid + 1, data);                    // uuid:1:uint64
                trace::protobuf::uint64(5, uuid, data);                        // parent_uuid:5:uint64
                {
                    const trace::protobuf::SubMessage counter(8, data);       // counter:8
                    trace::protobuf::int32(1, COUNTER_THREAD_TIME_NS, data);  // type:1:int32
                }
            }
        }
        write_trace_packet(data);

        return uuid;
    }

    const ReactionStatistics::Event& relevant_event(const ReactionEvent& event) {
        const auto& statistics = *event.statistics;
        switch (event.type) {
            case ReactionEvent::BLOCKED:
            case ReactionEvent::MISSING_DATA:
            case ReactionEvent::CREATED: return statistics.created;
            case ReactionEvent::STARTED: return statistics.started;
            case ReactionEvent::FINISHED: return statistics.finished;
            default: throw std::invalid_argument("Unknown event type");
        }
    }

    void TraceController::encode_event(const ReactionEvent& event) {

        const auto& relevant            = relevant_event(event);
        const auto& task_id             = event.statistics->target.task_id;
        const uint64_t thread_uuid      = thread(relevant.thread);
        const uint64_t thread_time_uuid = thread_uuid + 1;
        const auto& ids                 = event.statistics->identifiers;

        const std::string rname  = event.statistics == nullptr || event.statistics->identifiers == nullptr
                                       ? "PowerPlant"
                                       : event.statistics->identifiers->reactor;
        const int32_t event_type = event.type == ReactionEvent::STARTED    ? TYPE_SLICE_BEGIN
                                   : event.type == ReactionEvent::FINISHED ? TYPE_SLICE_END
                                                                           : TYPE_INSTANT;

        std::vector<char> data;
        {
            const trace::protobuf::SubMessage packet(1, data);                 // packet:1
            trace::protobuf::uint64(8, ts(relevant.real_time).count(), data);  // timestamp:8:uint64
            trace::protobuf::uint32(10, trusted_packet_sequence_id, data);     // trusted_packet_sequence_id:10:uint32
            trace::protobuf::int32(13, SEQ_NEEDS_INCREMENTAL_STATE, data);     // sequence_flags:13:int32
            {
                const trace::protobuf::SubMessage track_event(11, data);   // track_event:11
                trace::protobuf::int32(9, event_type, data);               // type:9:int32
                trace::protobuf::uint64(11, thread_uuid, data);            // track_uuid:11:uint64
                trace::protobuf::uint64(10, event_names[ids], data);       // name_iid:10:uint64
                trace::protobuf::uint64(3, categories[rname], data);       // category_iids:3:uint64
                trace::protobuf::uint64(3, categories["reaction"], data);  // category_iids:3:uint64
                trace::protobuf::uint64(31, thread_time_uuid, data);       // extra_counter_track_uuids:31:uint64
                trace::protobuf::int64(12, ts(relevant.thread_time).count(), data);  // extra_counter_values:12:int64
                if (event.type == ReactionEvent::CREATED || event.type == ReactionEvent::STARTED) {
                    trace::protobuf::uint64(47, task_id, data);  // flow_ids:47:fixed64
                }
            }
        }
        write_trace_packet(data);
    }

    void TraceController::encode_log(const std::shared_ptr<const message::ReactionStatistics>& log_stats,
                                     const LogMessage& msg) {

        const auto& msg_stats           = msg.statistics;
        const auto& created             = log_stats->created;
        const uint64_t thread_uuid      = thread(created.thread);
        const uint64_t thread_time_uuid = thread_uuid + 1;

        int32_t prio = PRIO_UNSPECIFIED;
        switch (msg.level) {
            case LogLevel::TRACE: prio = PRIO_VERBOSE; break;
            case LogLevel::DEBUG: prio = PRIO_DEBUG; break;
            case LogLevel::INFO: prio = PRIO_INFO; break;
            case LogLevel::WARN: prio = PRIO_WARN; break;
            case LogLevel::ERROR: prio = PRIO_ERROR; break;
            case LogLevel::FATAL: prio = PRIO_FATAL; break;
            default: break;
        }

        auto ids                = msg_stats != nullptr ? msg_stats->identifiers : nullptr;
        const std::string rname = ids == nullptr ? "PowerPlant" : msg_stats->identifiers->reactor;

        std::vector<char> data;
        {
            const trace::protobuf::SubMessage packet(1, data);
            trace::protobuf::uint64(8, ts(created.real_time).count(), data);  // timestamp:8:uint64
            trace::protobuf::uint32(10, trusted_packet_sequence_id, data);    // trusted_packet_sequence_id:10:uint32
            trace::protobuf::int32(13, SEQ_NEEDS_INCREMENTAL_STATE, data);    // sequence_flags:13:int32
            {
                const trace::protobuf::SubMessage track_event(11, data);  // track_event:11
                trace::protobuf::uint64(11, thread_uuid, data);           // track_uuid:11:uint64
                trace::protobuf::uint64(10, event_names[ids], data);      // name_iid:10:uint64
                trace::protobuf::uint64(3, categories[rname], data);      // category_iids:3:uint64
                trace::protobuf::uint64(3, categories["log"], data);      // category_iids:3:uint64
                trace::protobuf::int32(9, TYPE_INSTANT, data);            // type:9:int32
                trace::protobuf::uint64(31, thread_time_uuid, data);      // extra_counter_track_uuids:31:uint64
                trace::protobuf::int64(12, ts(created.thread_time).count(), data);  // extra_counter_values:12:int64
                {
                    const trace::protobuf::SubMessage log_message(21, data);            // log_message:21
                    trace::protobuf::uint64(2, log_message_bodies[msg.message], data);  // body_iid:2:uint64
                    trace::protobuf::int32(3, prio, data);                              // prio:3:int32
                }
            }
        }
        write_trace_packet(data);
    }

    TraceController::TraceController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment))
        , categories(  //
              trusted_packet_sequence_id,
              [](const auto& key) { return key; },
              [this](const auto& data) { write_trace_packet(data); })
        , event_names(  //
              trusted_packet_sequence_id,
              name_for_id,
              [this](const auto& data) { write_trace_packet(data); })
        , log_message_bodies(  //
              trusted_packet_sequence_id,
              [](const auto& key) { return key; },
              [this](const auto& data) { write_trace_packet(data); }) {

        on<Trigger<BeginTrace>, Pool<TracePool>>().then([this](const BeginTrace& e) {
            // Clean up any current trace
            event_handle.unbind();
            log_handle.unbind();
            trace_file.close();

            // Open a new file in the target location
            trace_file.open(e.file, std::ios::binary);

            // Write a reset packet so that incremental state works
            std::vector<char> data;
            {
                const trace::protobuf::SubMessage packet(1, data);
                trace::protobuf::uint32(10, trusted_packet_sequence_id, data);  // trusted_packet_sequence_id:10:uint32
                trace::protobuf::int32(87, 1, data);                            // first_packet_on_sequence:87:bool
                trace::protobuf::int32(42, 1, data);                            // previous_packet_dropped:42:bool
                trace::protobuf::int32(13, SEQ_INCREMENTAL_STATE_CLEARED, data);  // sequence flags:13:int32
            }
            write_trace_packet(data);

            // Write the trace events that happened before the trace started
            auto current_stats = threading::ReactionTask::get_current_task()->statistics;
            encode_event(ReactionEvent(ReactionEvent::CREATED, current_stats));
            encode_event(ReactionEvent(ReactionEvent::STARTED, current_stats));

            // Bind new handles
            event_handle = on<Trigger<ReactionEvent>, Pool<TracePool>>().then([this](const ReactionEvent& e) {  //
                encode_event(e);
            });
            if (e.logs) {
                log_handle =
                    on<Trigger<LogMessage>, Pool<TracePool>, Inline::NEVER>().then([this](const LogMessage& msg) {
                        // Statistics for the log message task itself
                        auto log_stats = threading::ReactionTask::get_current_task()->statistics;
                        encode_log(log_stats, msg);
                    });
            }
        });

        on<Trigger<EndTrace>, Pool<TracePool>>().then([this] {
            // Unbind the handles and close the file
            event_handle.unbind();
            log_handle.unbind();
            trace_file.close();
        });
    }

}  // namespace extension
}  // namespace NUClear
