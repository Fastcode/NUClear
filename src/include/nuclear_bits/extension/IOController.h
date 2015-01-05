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

#ifndef NUCLEAR_EXTENSION_IOCONTROLLER
#define NUCLEAR_EXTENSION_IOCONTROLLER

#include <algorithm>

extern "C" {
    #include <unistd.h>
    #include <poll.h>
}
    
namespace NUClear {
    namespace extension {
        
        class IOController : public Reactor {
        public:
            explicit IOController(std::unique_ptr<NUClear::Environment> environment)
            : Reactor(std::move(environment)) {
                
                int vals[2];
                
                int i = pipe(vals);
                if(i < 0) {
                    throw std::system_error(errno, std::system_category(), "We were unable to make the notification pipe for IO");
                }
                
                notifyRecv = vals[0];
                notifySend = vals[1];
                
                // Add our notification pipe to our list of fds
                fds.push_back(pollfd { notifyRecv, POLLIN, 0 });
                
                on<Trigger<dsl::word::IOConfiguration>>().then([this] (const dsl::word::IOConfiguration& config) {
                
                
                    std::cout << "Add the FD " << config.fd << " to the set" << std::endl;
                    
                });
                
                on<Trigger<dsl::word::UnbindIO>>().then([this] (const dsl::word::UnbindIO& unbind) {
                
                    std::cout << "TODO remove the FD from the set" << std::endl;
                });
                
                on<Shutdown>().then([this] {
                    
                    // A byte to send down the pipe
                    char val = 0;
                    
                    // Send a single byte down the pipe
                    write(notifySend, &val, 1);
                });
                
                on<Always>().then([this] {
                    
                    // Poll our file descriptors for events
                    int result = poll(fds.data(), fds.size(), -1);
                    
                    if(result < 0) {
                        throw std::system_error(errno, std::system_category(), "There was an IO error while attempting to poll the file descriptors");
                    }
                    
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                });
                
            }
            
        private:
            int notifyRecv;
            int notifySend;
            
            std::vector<pollfd> fds;
        };
    }
}

#endif
