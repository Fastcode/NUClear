/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

namespace NUClear {
    
    template <typename TReactor, enum LogLevel level>
    void PowerPlant::install() {
        
        // Make sure that the class that we received is a reactor
        static_assert(std::is_base_of<Reactor, TReactor>::value, "You must install Reactors");
        
        // The reactor constructor should handle subscribing to events
        reactors.push_back(std::make_unique<TReactor>(std::make_unique<Environment>(*this, level)));
    }
    
    // Default emit with no types
    template <typename TData>
    void PowerPlant::emit(std::unique_ptr<TData>&& data) {
        
        emit<dsl::word::emit::Local>(std::forward<std::unique_ptr<TData>>(data));
    }
    // Default emit with no types
    template <typename TData>
    void PowerPlant::emit(std::unique_ptr<TData>& data) {
        
        emit<dsl::word::emit::Local>(std::move(data));
    }
    
    // Default emit with no types
    template <template <typename> class TFirstHandler, template <typename> class... THandlers, typename TData, typename... TArgs>
    void PowerPlant::emit(std::unique_ptr<TData>& data, TArgs&&... args) {
        
        emit<TFirstHandler, THandlers...>(std::move(data), std::forward<TArgs>(args)...);
    }
    
    template <typename Handler>
    struct EmitCaller {
        template <typename... TArgs>
        static inline auto call(TArgs&&... args)
        -> decltype(Handler::emit(std::forward<TArgs>(args)...), true) {
            Handler::emit(std::forward<TArgs>(args)...);
            return true;
        }
    };
    
    // Global emit handlers
    template <template <typename> class TFirstHandler, template <typename> class... THandlers, typename TData,  typename... TArgs>
    void PowerPlant::emit(std::unique_ptr<TData>&& data, TArgs&&... args) {
        
        // Release our data from the pointer and wrap it in a shared_ptr
        std::shared_ptr<TData> ptr = std::shared_ptr<TData>(std::move(data));
        
        // Provide a check to make sure they are passing us the right stuff
        static_assert(util::FunctionFusion<std::tuple<TFirstHandler<TData>, THandlers<TData>...>
                      , decltype(std::forward_as_tuple(*this, ptr, std::forward<TArgs>(args)...))
                      , EmitCaller
                      , 2>::value,
                      "There was an error with the arguments for the emit function, Check that your scope and arguments match what you are trying to do.");
        
        // Fuse our emit handlers and call the fused function
        util::FunctionFusion<std::tuple<TFirstHandler<TData>, THandlers<TData>...>
        , decltype(std::forward_as_tuple(*this, ptr, std::forward<TArgs>(args)...))
        , EmitCaller
        , 2>::call(*this, ptr, std::forward<TArgs>(args)...);
    }
    
    // Anonymous metafunction that concatenates everything into a single string
    namespace {
        template <typename TFirst>
        inline void logImpl(std::stringstream& output, TFirst&& first) {
            output << first;
        }
        
        template <typename TFirst, typename... TArgs>
        inline void logImpl(std::stringstream& output, TFirst&& first, TArgs&&... args) {
            output << first << " ";
            logImpl(output, std::forward<TArgs>(args)...);
        }
    }
    
    template <enum LogLevel level, typename... TArgs>
    void PowerPlant::log(TArgs&&... args) {
            
        // Build our log message by concatenating everything to a stream
        std::stringstream outputStream;
        logImpl(outputStream, std::forward<TArgs>(args)...);
        std::string output = outputStream.str();
        
        // Direct emit the log message so that any direct loggers can use it
        powerplant->emit<dsl::word::emit::Direct>(std::make_unique<message::LogMessage>(level
                                                                          , output
                                                                          , threading::ReactionTask::getCurrentTask()->stats.get()));
    }
}
