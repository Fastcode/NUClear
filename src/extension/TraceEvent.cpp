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

#include "TraceEvent.hpp"

#include "../message/LogMessage.hpp"
#include "../message/ReactionStatistics.hpp"

namespace NUClear {
namespace extension {

    using message::LogMessage;
    using message::ReactionEvent;
    using message::ReactionStatistics;

    const ReactionStatistics::Event& relevant_event(const ReactionEvent::Event& event,
                                                    const ReactionStatistics& statistics) {
        switch (event) {
            case ReactionEvent::BLOCKED:
            case ReactionEvent::MISSING_DATA:
            case ReactionEvent::CREATED: return statistics.created;
            case ReactionEvent::STARTED: return statistics.started;
            case ReactionEvent::FINISHED: return statistics.finished;
            default: throw std::runtime_error("Unknown event type");
        }
    }

    template <typename T>
    std::chrono::microseconds us(const T& duration) {
        return std::chrono::duration_cast<std::chrono::microseconds>(duration);
    }

    std::string timestamp(const ReactionStatistics::Event& event) {
        std::stringstream ss;
        ss << ", \"ts\": " << us(event.realtime.time_since_epoch()).count();
        ss << ", \"tts\": " << us(event.cpu_time.time_since_epoch()).count();
        return ss.str();
    }


    std::string TraceEvent::thread_info(const ReactionStatistics::Event::ThreadInfo& info) {

        std::lock_guard<std::mutex> lock(threads_mutex);
        if (pools.emplace(info.pool, pools.size() + 1).second) {
            /*
             * Create a metadata event when a pool is created
             * {
             *     "name": "process_name",
             *     "ph": "M",
             *     "pid": "pool_id",
             *     "tid": "0",
             *     "ts": "0",
             *     "args": {
             *         "name": "pool_name"
             *     }
             * }
             */
            std::string name = info.pool == nullptr ? "Non NUClear" : info.pool->name;

            std::stringstream ss;
            ss << "{";
            ss << "\"name\": \"process_name\"";
            ss << ", \"ph\": \"M\"";
            ss << ", \"pid\": " << pools.at(info.pool) << "";
            ss << ", \"tid\": " << 0 << "";
            ss << ", \"ts\": \"" << 0 << "\"";
            ss << ", \"args\": {\"name\": \"" << name << "\"}";
            ss << "},";
            const std::lock_guard<std::mutex> lock(trace_mutex);
            trace_file << ss.str() << std::endl;
        }

        // Assigns an integer to each thread if it hasn't been seen before
        threads.emplace(info.thread_id, threads.size() + 1);

        std::stringstream ss;
        ss << ", \"pid\": " << pools.at(info.pool) << ", \"tid\": " << threads.at(info.thread_id);
        return ss.str();
    }

    void TraceEvent::duration_event(const ReactionEvent& event) {

        char phase{};
        switch (event.type) {
            case ReactionEvent::STARTED: phase = 'B'; break;
            case ReactionEvent::FINISHED: phase = 'E'; break;
            case ReactionEvent::CREATED:
            case ReactionEvent::BLOCKED:
            case ReactionEvent::MISSING_DATA: phase = 'i'; break;
            default: throw std::runtime_error("Unknown event type");
        }

        auto& relevant = relevant_event(event.type, *event.statistics);

        auto ids            = event.statistics->identifiers;
        std::string name    = ids->name.empty() ? ids->dsl : ids->name;
        std::string reactor = ids->reactor;

        std::stringstream ss;
        ss << "{";

        ss << "\"name\": \"" << name << "\"";
        ss << ", \"id\": \"" << event.statistics->target.task_id << "\"";
        ss << ", \"cat\": \"" << reactor << ",reaction\"";

        ss << ", \"ph\": \"" << phase << "\"";

        if (event.type == ReactionEvent::BLOCKED) {
            ss << ", \"args\": {\"reason\": \"BlOCKED\"}}";
        }
        else if (event.type == ReactionEvent::MISSING_DATA) {
            ss << ", \"args\": {\"reason\": \"MISSING_DATA\"}}";
        }

        ss << timestamp(relevant);
        ss << thread_info(relevant.thread);

        ss << "},";

        const std::lock_guard<std::mutex> lock(trace_mutex);
        trace_file << ss.str() << std::endl;
    }

    void TraceEvent::flow_event(const ReactionEvent& event) {

        char phase{};
        switch (event.type) {
            case ReactionEvent::CREATED: phase = 's'; break;
            case ReactionEvent::STARTED: phase = 'f'; break;
            default: return;
        }

        auto& relevant = relevant_event(event.type, *event.statistics);

        auto ids            = event.statistics->identifiers;
        std::string name    = ids->name.empty() ? ids->dsl : ids->name;
        std::string reactor = ids->reactor;

        std::stringstream ss;
        ss << "{";

        ss << "\"name\": \"" << name << "\"";
        ss << ", \"id\": \"" << event.statistics->target.task_id << "\"";
        ss << ", \"cat\": \"" << reactor << ",reaction\"";

        ss << ", \"ph\": \"" << phase << "\"";
        ss << ", \"bp\": \"e\"";

        ss << timestamp(relevant);
        ss << thread_info(relevant.thread);

        ss << "},";

        const std::lock_guard<std::mutex> lock(trace_mutex);
        trace_file << ss.str() << std::endl;
    }

    TraceEvent::TraceEvent(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        trace_file.open("trace.json");
        trace_file << "{\n\"traceEvents\":[" << std::endl;

        on<Trigger<ReactionEvent>>().then([this](const ReactionEvent& event) {
            duration_event(event);
            flow_event(event);
        });

        on<Trigger<LogMessage>>().then([this](const LogMessage& message) {
            /*
             * Create an instant event for any log messages
             *
             * {
             *     "name": "log",
             *     "cat": ["log"],
             *     "ph": "i",
             *     "ts": steady_timestamp,
             *     "tts": thread_timestamp,
             *     "pid": "pool_id",
             *     "tid": "thread_id",
             *     "args": {
             *         "level": "log_level",
             *         "message": "log_message"
             *     }
             * }
             */

            std::stringstream ss;

            auto ids          = message.statistics != nullptr ? message.statistics->identifiers : nullptr;
            std::string name  = ids == nullptr      ? ""  //
                                : ids->name.empty() ? ids->dsl
                                                    : ids->name;
            std::string scope = ids == nullptr ? "PowerPlant" : ids->reactor;

            auto now = ReactionStatistics::Event::now();

            ss << "{";

            ss << "\"name\": \"log\"";
            ss << ", \"cat\": \"" << scope << "" << ",log\"";

            ss << ", \"ph\": \"i\"";

            ss << timestamp(now);
            ss << thread_info(now.thread);

            ss << ", \"args\": {";
            ss << "\"level\": \"" << message.level << "\"";
            ss << ", \"message\": \"" << message.message << "\"";
            ss << "}";
            ss << "},";

            std::lock_guard<std::mutex> lock(trace_mutex);
            trace_file << ss.str() << std::endl;
        });
    }

    TraceEvent::~TraceEvent() {
        // Delete the last comma
        trace_file.seekp(-1, std::ios_base::end);
        trace_file << "]}";
        trace_file.close();
    }

}  // namespace extension
}  // namespace NUClear
