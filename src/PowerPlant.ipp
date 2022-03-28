/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include "extension/ChronoController.hpp"
#include "extension/IOController.hpp"
#include "extension/NetworkController.hpp"

namespace NUClear {

inline PowerPlant::PowerPlant(Configuration config, int argc, const char* argv[]) : configuration(config) {

    // Stop people from making more then one powerplant
    if (powerplant != nullptr) {
        throw std::runtime_error("There is already a powerplant in existence (There should be a single PowerPlant)");
    }

    // Store our static variable
    powerplant = this;

    // Install the Chrono reactor
    install<extension::ChronoController>();
    install<extension::IOController>();
    install<extension::NetworkController>();

    // Emit our arguments if any.
    message::CommandLineArguments args;
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    // We emit this twice, so the data is available for extensions
    emit(std::make_unique<message::CommandLineArguments>(args));
    emit<dsl::word::emit::Initialise>(std::make_unique<message::CommandLineArguments>(args));
}

template <typename T, enum LogLevel level>
void PowerPlant::install() {

    // Make sure that the class that we received is a reactor
    static_assert(std::is_base_of<Reactor, T>::value, "You must install Reactors");

    // The reactor constructor should handle subscribing to events
    reactors.push_back(
        std::make_unique<T>(std::make_unique<Environment>(*this, util::demangle(typeid(T).name()), level)));
}

// Default emit with no types
template <typename T>
void PowerPlant::emit(std::unique_ptr<T>&& data) {

    emit<dsl::word::emit::Local>(std::move(data));
}
// Default emit with no types
template <typename T>
void PowerPlant::emit(std::unique_ptr<T>& data) {

    emit<dsl::word::emit::Local>(std::move(data));
}

// Default emit with no types
template <template <typename> class First, template <typename> class... Remainder, typename T, typename... Arguments>
void PowerPlant::emit(std::unique_ptr<T>& data, Arguments&&... args) {

    emit<First, Remainder...>(std::move(data), std::forward<Arguments>(args)...);
}

/**
 * @brief This is our Function Fusion wrapper class that allows it to call emit functions
 *
 * @tparam Handler The emit handler that we are wrapping for
 */
template <typename Handler>
struct EmitCaller {
    template <typename... Arguments>
    static inline auto call(Arguments&&... args)
        // THIS IS VERY IMPORTANT, the return type must be dependent on the function call
        // otherwise it won't check it's valid in SFINAE (the comma operator does it again!)
        -> decltype(Handler::emit(std::forward<Arguments>(args)...), true) {
        Handler::emit(std::forward<Arguments>(args)...);
        return true;
    }
};

// Global emit handlers


template <template <typename> class First, template <typename> class... Remainder, typename T, typename... Arguments>
void PowerPlant::emit_shared(std::shared_ptr<T>&& ptr, Arguments&&... args) {

    using Functions      = std::tuple<First<T>, Remainder<T>...>;
    using ArgumentPack   = decltype(std::forward_as_tuple(*this, ptr, std::forward<Arguments>(args)...));
    using CallerArgs     = std::tuple<>;
    using FusionFunction = util::FunctionFusion<Functions, ArgumentPack, EmitCaller, CallerArgs, 2>;

    // Provide a check to make sure they are passing us the right stuff
    static_assert(FusionFunction::value,
                  "There was an error with the arguments for the emit function, Check that your scope and arguments "
                  "match what you are trying to do.");

    // Fuse our emit handlers and call the fused function
    FusionFunction::call(*this, ptr, std::forward<Arguments>(args)...);
}

template <template <typename> class First, template <typename> class... Remainder, typename... Arguments>
void PowerPlant::emit_shared(Arguments&&... args) {

    using Functions      = std::tuple<First<void>, Remainder<void>...>;
    using ArgumentPack   = decltype(std::forward_as_tuple(*this, std::forward<Arguments>(args)...));
    using CallerArgs     = std::tuple<>;
    using FusionFunction = util::FunctionFusion<Functions, ArgumentPack, EmitCaller, CallerArgs, 1>;

    // Provide a check to make sure they are passing us the right stuff
    static_assert(FusionFunction::value,
                  "There was an error with the arguments for the emit function, Check that your scope and arguments "
                  "match what you are trying to do.");

    // Fuse our emit handlers and call the fused function
    FusionFunction::call(*this, std::forward<Arguments>(args)...);
}

template <template <typename> class First, template <typename> class... Remainder, typename T, typename... Arguments>
void PowerPlant::emit(std::unique_ptr<T>&& data, Arguments&&... args) {

    // Release our data from the pointer and wrap it in a shared_ptr
    emit_shared<First, Remainder...>(std::shared_ptr<T>(std::move(data)), std::forward<Arguments>(args)...);
}

template <template <typename> class First, template <typename> class... Remainder, typename... Arguments>
void PowerPlant::emit(Arguments&&... args) {

    emit_shared<First, Remainder...>(std::forward<Arguments>(args)...);
}

// Anonymous metafunction that concatenates everything into a single string
namespace {
    template <typename T>
    inline void log_impl(std::stringstream& output, T&& first) {
        output << first;
    }

    template <typename First, typename... Remainder>
    inline void log_impl(std::stringstream& output, First&& first, Remainder&&... args) {
        output << first << " ";
        log_impl(output, std::forward<Remainder>(args)...);
    }
}  // namespace

template <enum LogLevel level, typename... Arguments>
void PowerPlant::log(Arguments&&... args) {

    // Build our log message by concatenating everything to a stream
    std::stringstream output_stream;
    log_impl(output_stream, std::forward<Arguments>(args)...);
    std::string output = output_stream.str();

    auto* current_task = threading::ReactionTask::get_current_task();
    auto* task         = current_task ? current_task->stats.get() : nullptr;

    // Direct emit the log message so that any direct loggers can use it
    powerplant->emit<dsl::word::emit::Direct>(
        std::make_unique<message::LogMessage>(message::LogMessage{level, output, task}));
}

}  // namespace NUClear
