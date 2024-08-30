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

#include "TraceController.hpp"

#include <chrono>
#include <regex>

#include "../message/LogMessage.hpp"
#include "../message/ReactionStatistics.hpp"
#include "../message/Trace.hpp"

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

    namespace {

        template <typename T>
        std::chrono::nanoseconds ts(const T& timestamp) {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch());
        }

        namespace encode {
            void uint64(const uint64_t& value, std::vector<char>& data) {
                // Encode the value as a varint
                uint64_t v = value;
                while (v >= 0x80) {
                    data.push_back((v & 0x7F) | 0x80);
                    v >>= 7;
                }
                data.push_back(v & 0x7F);
            }

            void int64(const int64_t& value, std::vector<char>& data) {
                // Encode the value as normal varint
                uint64(uint64_t(value), data);
            }

            void fixed64(const uint64_t& value, std::vector<char>& data) {
                // Encode the value as a little endian fixed 64
                data.push_back((value >> 0) & 0xFF);
                data.push_back((value >> 8) & 0xFF);
                data.push_back((value >> 16) & 0xFF);
                data.push_back((value >> 24) & 0xFF);
                data.push_back((value >> 32) & 0xFF);
                data.push_back((value >> 40) & 0xFF);
                data.push_back((value >> 48) & 0xFF);
                data.push_back((value >> 56) & 0xFF);
            }

            void uint32(const uint32_t& value, std::vector<char>& data) {
                // Encode the value as a varint
                uint32_t v = value;
                while (v >= 0x80) {
                    data.push_back((v & 0x7F) | 0x80);
                    v >>= 7;
                }
                data.push_back(v & 0x7F);
            }

            void int32(const int32_t& value, std::vector<char>& data) {
                // Encode the value as a zigzag varint
                uint32(value, data);
            }

            void string(const std::string& value, std::vector<char>& data) {
                // Encode the value as a varint length and then the string
                uint64(value.size(), data);
                data.insert(data.end(), value.begin(), value.end());
            }
        }  // namespace encode

        namespace field {
            void uint64(const uint32_t& id, const uint64_t& value, std::vector<char>& data) {
                encode::uint32(id << 3 | 0, data);
                encode::uint64(value, data);
            }

            void int64(const uint32_t& id, const int64_t& value, std::vector<char>& data) {
                encode::uint32(id << 3 | 0, data);
                encode::int64(value, data);
            }

            void fixed64(const uint32_t& id, const uint64_t& value, std::vector<char>& data) {
                encode::uint32(id << 3 | 1, data);
                encode::fixed64(value, data);
            }

            void uint32(const uint32_t& id, const uint32_t& value, std::vector<char>& data) {
                encode::uint32(id << 3 | 0, data);
                encode::uint32(value, data);
            }

            void int32(const uint32_t& id, const int32_t& value, std::vector<char>& data) {
                encode::uint32(id << 3 | 0, data);
                encode::int32(value, data);
            }

            void string(const uint32_t& id, const std::string& value, std::vector<char>& data) {
                encode::uint32(id << 3 | 2, data);
                encode::string(value, data);
            }
        }  // namespace field
    }  // namespace


    class SubMessage {
    public:
        SubMessage(const uint32_t& id, std::vector<char>& data) : data(data) {
            encode::uint32(id << 3 | 2, data);  // Type and id
            data.push_back(0);                  // Placeholder for the length
            data.push_back(0);                  // Placeholder for the length
            start = data.size();                // Store the current position so we can write the length later
        }
        ~SubMessage() {
            // Update the length of the SubMessage
            auto len        = redundant_varint<2>(uint32_t(data.size() - start));
            data[start - 2] = len[0];
            data[start - 1] = len[1];
        }
        SubMessage(const SubMessage&)            = delete;
        SubMessage(SubMessage&&)                 = delete;
        SubMessage& operator=(const SubMessage&) = delete;
        SubMessage& operator=(SubMessage&&)      = delete;

        template <size_t N>
        static std::array<char, N> redundant_varint(const uint32_t value) {
            // Encode the value as a varint
            uint32_t v = value;
            std::array<char, N> result;
            for (unsigned i = 0; i < N - 1; i++) {
                result[i] = (v & 0x7F) | 0x80;
                v >>= 7;
            }
            result[N - 1] = v & 0x7F;

            return result;
        }

    private:
        std::vector<char>& data;
        size_t start;
    };

    std::string name_for_id(const std::shared_ptr<const NUClear::threading::ReactionIdentifiers>& ids) {
        if (ids == nullptr) {
            // With no identifiers we can't do anything
            return "";
        }
        else if (ids->name.empty()) {
            // Remove the namespace from the DSL with a regex
            return std::regex_replace(ids->dsl, std::regex("[A-Za-z_][A-Za-z0-9_]+::"), "");
        }
        else {
            return ids->name;
        }
    }

    uint64_t TraceController::category(const std::string& name) {
        if (category_iids.find(name) != category_iids.end()) {
            return category_iids.at(name);
        }

        uint64_t iid = category_iids[name] = category_iids.size() + 1;
        std::vector<char> data;
        {
            SubMessage packet(1, data);                           // packet:1
            field::uint32(10, trusted_packet_sequence_id, data);  // trusted_packet_sequence_id:10:uint32
            {
                SubMessage interned_data(12, data);  // interned_data:12
                {
                    SubMessage event_categories(1, data);  // event_categories:1
                    field::uint64(1, iid, data);           // iid:1:uint64
                    field::string(2, name, data);          // name:2:string
                }
            }
        }
        trace_file.write(data.data(), data.size());

        return iid;
    }

    uint64_t TraceController::event_name(const std::shared_ptr<const NUClear::threading::ReactionIdentifiers>& ids) {
        if (event_name_iids.find(ids) != event_name_iids.end()) {
            return event_name_iids.at(ids);
        }

        std::string name = name_for_id(ids);

        uint64_t iid = event_name_iids[ids] = event_name_iids.size() + 1;
        std::vector<char> data;
        {
            SubMessage packet(1, data);                           // packet:1
            field::uint32(10, trusted_packet_sequence_id, data);  // trusted_packet_sequence_id:10:uint32
            {
                SubMessage interned_data(12, data);  // interned_data:12
                {
                    SubMessage event_names(2, data);  // event_names:2
                    field::uint64(1, iid, data);      // iid:1:uint64
                    field::string(2, name, data);     // name:2:string
                }
            }
        }
        trace_file.write(data.data(), data.size());

        return iid;
    }

    uint64_t TraceController::source_location(
        const std::shared_ptr<const NUClear::threading::ReactionIdentifiers>& ids) {
        if (source_location_iids.find(ids) != source_location_iids.end()) {
            return source_location_iids.at(ids);
        }

        std::string name = name_for_id(ids);

        uint64_t iid = source_location_iids[ids] = source_location_iids.size() + 1;
        std::vector<char> data;
        {
            SubMessage packet(1, data);                           // packet:1
            field::uint32(10, trusted_packet_sequence_id, data);  // trusted_packet_sequence_id:10:uint32
            {
                SubMessage interned_data(12, data);  // interned_data:12
                {
                    SubMessage source_locations(4, data);  // event_names:2
                    field::uint64(1, iid, data);           // iid:1:uint64
                    field::string(2, name, data);          // name:2:string
                }
            }
        }
        trace_file.write(data.data(), data.size());

        return iid;
    }

    uint64_t TraceController::log_message_body(const std::string& name) {
        if (log_message_body_iids.find(name) != log_message_body_iids.end()) {
            return log_message_body_iids.at(name);
        }

        uint64_t iid = log_message_body_iids[name] = log_message_body_iids.size() + 1;
        std::vector<char> data;
        {
            SubMessage packet(1, data);                           // packet:1
            field::uint32(10, trusted_packet_sequence_id, data);  // trusted_packet_sequence_id:10:uint32
            {
                SubMessage interned_data(12, data);  // interned_data:12
                {
                    SubMessage log_message_body(20, data);  // log_message_body:20
                    field::uint64(1, iid, data);            // iid:1:uint64
                    field::string(2, name, data);           // name:2:string
                }
            }
        }
        trace_file.write(data.data(), data.size());

        return iid;
    }

    uint64_t TraceController::nuclear_process() {
        if (nuclear_process_uuid == 0) {
            nuclear_process_uuid = 1;
            std::vector<char> data;
            {
                SubMessage packet(1, data);  // packet:1
                {
                    SubMessage track_descriptor(60, data);  // track_descriptor:60
                    field::uint64(1, 1, data);              // uuid:1:uint64
                    {
                        SubMessage process(3, data);        // process:3
                        field::int32(1, 1, data);           // pid:1:int32
                        field::string(6, "NUClear", data);  // name:6:string
                    }
                }
            }
            trace_file.write(data.data(), data.size());
        }

        return nuclear_process_uuid;
    }

    uint64_t TraceController::non_nuclear_process() {
        if (non_nuclear_process_uuid == 0) {
            non_nuclear_process_uuid = 2;
            std::vector<char> data;
            {
                SubMessage packet(1, data);  // packet:1
                {
                    SubMessage track_descriptor(60, data);  // track_descriptor:60
                    field::uint64(1, 1, data);              // uuid:1:uint64
                    {
                        SubMessage process(3, data);            // process:3
                        field::int32(1, 2, data);               // pid:1:int32
                        field::string(6, "Non NUClear", data);  // name:6:string
                    }
                }
            }
            trace_file.write(data.data(), data.size());
        }
        return non_nuclear_process_uuid;
    }

    uint64_t TraceController::thread(const ReactionStatistics::Event::ThreadInfo& info) {
        if (thread_uuids.find(info.thread_id) != thread_uuids.end()) {
            return thread_uuids.at(info.thread_id);
        }

        auto parent_uuid = info.pool != nullptr ? nuclear_process() : non_nuclear_process();
        uint64_t uuid = thread_uuids[info.thread_id] = next_uuid.fetch_add(2, std::memory_order::memory_order_relaxed);
        std::string name                             = info.pool == nullptr ? "Non NUClear" : info.pool->name;

        std::vector<char> data;
        {
            SubMessage packet(1, data);  // packet:1
            {
                SubMessage track_descriptor(60, data);  // track_descriptor:60
                field::uint64(1, uuid, data);           // uuid:1:uint64
                field::uint64(5, parent_uuid, data);    // parent_uuid:5:uint64
                {
                    SubMessage thread(4, data);                   // thread:4
                    field::int32(1, int32_t(parent_uuid), data);  // pid:1:int32
                    field::int32(2, int32_t(uuid), data);         // tid:2:int32
                    field::string(5, name, data);                 // name:5:string
                }
            }
        }
        {
            SubMessage packet(1, data);  // packet:1
            {
                SubMessage track_descriptor(60, data);  // track_descriptor:60
                field::uint64(1, uuid + 1, data);       // uuid:1:uint64
                field::uint64(5, uuid, data);           // parent_uuid:5:uint64
                {
                    SubMessage counter(8, data);                    // counter:8
                    field::int32(1, COUNTER_THREAD_TIME_NS, data);  // type:1:int32
                }
            }
        }
        trace_file.write(data.data(), data.size());

        return uuid;
    }

    const ReactionStatistics::Event& relevant_event(const ReactionEvent& event) {
        auto& statistics = *event.statistics;
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

        const auto& relevant  = relevant_event(event);
        auto task_id          = event.statistics->target.task_id;
        auto thread_uuid      = thread(relevant.thread);
        auto thread_time_uuid = thread_uuid + 1;
        const auto& ids       = event.statistics->identifiers;

        auto name         = ids->name;
        std::string rname = event.statistics == nullptr || event.statistics->identifiers == nullptr
                                ? "PowerPlant"
                                : event.statistics->identifiers->reactor;

        std::vector<char> data;
        {
            SubMessage packet(1, data);                             // packet:1
            field::uint64(8, ts(relevant.realtime).count(), data);  // timestamp:8:uint64
            field::uint32(10, trusted_packet_sequence_id, data);    // trusted_packet_sequence_id:10:uint32
            field::int32(13, SEQ_NEEDS_INCREMENTAL_STATE, data);    // sequence_flags:13:int32
            {
                SubMessage track_event(11, data);                       // track_event:11
                field::uint64(11, thread_uuid, data);                   // track_uuid:11:uint64
                field::uint64(10, event_name(ids), data);               // name_iid:10:uint64
                field::uint64(3, category(rname), data);                // category_iids:3:uint64
                field::uint64(3, category("reaction"), data);           // category_iids:3:uint64
                field::uint64(31, thread_time_uuid, data);              // extra_counter_track_uuids:31:uint64
                field::int64(12, ts(relevant.cpu_time).count(), data);  // extra_counter_values:12:int64

                switch (event.type) {
                    case ReactionEvent::CREATED: {
                        field::int32(9, TYPE_INSTANT, data);  // type:9:int32
                        field::fixed64(47, task_id, data);    // flow_ids:47:fixed64
                    } break;
                    case ReactionEvent::STARTED: {
                        field::int32(9, TYPE_SLICE_BEGIN, data);  // type:9:int32
                        field::fixed64(47, task_id, data);        // flow_ids:47:fixed64
                    } break;
                    case ReactionEvent::FINISHED: {
                        field::int32(9, TYPE_SLICE_END, data);  // type:9:int32
                    } break;
                    case ReactionEvent::BLOCKED:
                    case ReactionEvent::MISSING_DATA: {
                        field::int32(9, TYPE_INSTANT, data);  // type:9:int32
                        // TODO extra data about the block
                    } break;
                }
            }
        }
        trace_file.write(data.data(), data.size());
    }

    void TraceController::encode_log(const std::shared_ptr<const message::ReactionStatistics>& log_stats,
                                     const LogMessage& msg) {

        const auto& msg_stats = msg.statistics;
        auto thread_uuid      = thread(log_stats->created.thread);

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

        auto ids          = msg_stats != nullptr ? msg_stats->identifiers : nullptr;
        std::string rname = ids == nullptr ? "PowerPlant" : msg_stats->identifiers->reactor;

        std::vector<char> data;
        {
            SubMessage packet(1, data);
            field::uint64(8, ts(log_stats->created.realtime).count(), data);  // timestamp:8:uint64
            field::uint32(10, trusted_packet_sequence_id,
                          data);                                  // trusted_packet_sequence_id:10:uint32
            field::int32(13, SEQ_NEEDS_INCREMENTAL_STATE, data);  // sequence_flags:13:int32
            {
                SubMessage track_event(11, data);          // track_event:11
                field::uint64(11, thread_uuid, data);      // track_uuid:11:uint64
                field::uint64(10, event_name(ids), data);  // name_iid:10:uint64
                field::uint64(3, category(rname), data);   // category_iids:3:uint64
                field::uint64(3, category("log"), data);   // category_iids:3:uint64
                field::int32(9, TYPE_INSTANT, data);       // type:9:int32
                {
                    SubMessage log_message(21, data);                       // log_message:21
                    field::uint64(1, source_location(ids), data);           // source_location_iid:1:uint64
                    field::uint64(2, log_message_body(msg.message), data);  // body_iid:2:uint64
                    field::int32(3, prio, data);                            // prio:3:int32
                }
            }
        }
        trace_file.write(data.data(), data.size());
    }

    TraceController::TraceController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

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
                SubMessage packet(1, data);
                field::uint32(10, trusted_packet_sequence_id, data);    // trusted_packet_sequence_id:10:uint32
                field::int32(87, true, data);                           // first_packet_on_sequence:87:bool
                field::int32(42, true, data);                           // previous_packet_dropped:42:bool
                field::int32(13, SEQ_INCREMENTAL_STATE_CLEARED, data);  // sequence flags:13:int32
            }
            trace_file.write(data.data(), data.size());

            // Bind new handles
            event_handle = on<Trigger<ReactionEvent>, Pool<TracePool>>().then([this](const ReactionEvent& e) {  //
                encode_event(e);
            });
            if (e.logs) {
                log_handle =
                    on<Trigger<LogMessage>, Pool<TracePool>, Inline::NEVER>().then([this](const LogMessage& msg) {
                        // Statistics for the log message task itself
                        auto current_task = threading::ReactionTask::get_current_task();
                        auto log_stats    = threading::ReactionTask::get_current_task()->stats;
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
