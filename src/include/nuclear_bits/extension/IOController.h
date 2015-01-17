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
        private:
            
            struct Task {
                int fd;
                short events;
                std::shared_ptr<threading::Reaction> reaction;
                
                bool operator< (const Task& other) const {
                    return fd == other.fd ? events < other.events : fd < other.fd;
                }
            };
            
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
                    
                    // Lock our mutex to avoid concurrent modification
                    std::lock_guard<std::mutex> lock(reactionMutex);
                    
                    // Work out our event mask
                    short events = 0;
                    
                    events |= config.events & dsl::word::IO::READ  ? POLLIN             : 0;
                    events |= config.events & dsl::word::IO::WRITE ? POLLOUT            : 0;
                    events |= config.events & dsl::word::IO::CLOSE ? POLLHUP            : 0;
                    events |= config.events & dsl::word::IO::ERROR ? POLLNVAL | POLLERR : 0;
                    
                    reactions.push_back(Task {
                        config.fd,
                        events,
                        config.reaction
                    });
                    
                    // Resort our list
                    std::sort(std::begin(reactions), std::end(reactions));
                    
                    // Let the poll command know that stuff happened
                    dirty = true;
                    
                    // Check if there was an error
                    if(write(notifySend, &dirty, 1) < 0) {
                        throw std::system_error(errno, std::system_category(), "There was an error while writing to the notification pipe");
                    }
                });
                
                on<Trigger<dsl::operation::Unbind<IO>>>().then([this] (const dsl::operation::Unbind<IO>& unbind) {
                    
                    // Lock our mutex to avoid concurrent modification
                    std::lock_guard<std::mutex> lock(reactionMutex);
                    
                    // Find our reaction
                    auto reaction = std::find_if(std::begin(reactions), std::end(reactions), [&unbind] (const Task& t) {
                        return t.reaction->reactionId == unbind.reactionId;
                    });
                    
                    if(reaction != std::end(reactions)) {
                        reactions.erase(reaction);
                    }
                    
                    // Let the poll command know that stuff happened
                    dirty = true;
                    if(write(notifySend, &dirty, 1) < 0) {
                        throw std::system_error(errno, std::system_category(), "There was an error while writing to the notification pipe");
                    }
                });
                
                on<Shutdown>().then([this] {
                    
                    // A byte to send down the pipe
                    char val = 0;
                    
                    // Send a single byte down the pipe
                    if(write(notifySend, &val, 1) < 0) {
                        throw std::system_error(errno, std::system_category(), "There was an error while writing to the notification pipe");
                    }
                });
                
                on<Always>().then([this] {
                    
                    // Poll our file descriptors for events
                    int result = poll(fds.data(), fds.size(), -1);
                    
                    // Check if we had an error on our Poll request
                    if(result < 0) {
                        throw std::system_error(errno, std::system_category(), "There was an IO error while attempting to poll the file descriptors");
                    }
                    else {
                        for(auto& fd : fds) {
                            
                            // Something happened
                            if(fd.revents) {
                                
                                // It's our notification handle
                                if(fd.fd == notifyRecv) {
                                    // Read our value to clear it's read status
                                    char val;
                                    read(fd.fd, &val, sizeof(char));
                                }
                                // It's a regular handle
                                else {
                                    
                                    // Find our relevant reactions
                                    auto range = std::equal_range(std::begin(reactions)
                                                                  , std::end(reactions)
                                                                  , Task { fd.fd, 0, nullptr }
                                                                  , [] (const Task& a, const Task& b) {
                                                                      return a.fd < b.fd;
                                                                  });
                                    
                                    
                                    // There are no reactions for this!
                                    if(range.first == std::end(reactions)) {
                                        // TODO maybe we should remove this from our set?
                                    }
                                    else {
                                        
                                        // Loop through our values
                                        for(auto it = range.first; it != range.second; ++it) {
                                            
                                            // We should emit if the reaction is interested
                                            if (it->events & fd.revents) {
                                                // Add the task to our queue
                                                try {
                                                    // Store our fd in thread store 1
                                                    dsl::store::ThreadStore<int, 0>::value = fd.fd;
                                                    
                                                    // Evaluate and store our set in thread store 2
                                                    dsl::store::ThreadStore<int, 1>::value = 0;
                                                    dsl::store::ThreadStore<int, 1>::value |= fd.revents & POLLIN               ? dsl::word::IO::READ     : 0;
                                                    dsl::store::ThreadStore<int, 1>::value |= fd.revents & POLLOUT              ? dsl::word::IO::WRITE    : 0;
                                                    dsl::store::ThreadStore<int, 1>::value |= fd.revents & POLLHUP              ? dsl::word::IO::CLOSE    : 0;
                                                    dsl::store::ThreadStore<int, 1>::value |= fd.revents & (POLLNVAL | POLLERR) ? dsl::word::IO::ERROR    : 0;
                                                    
                                                    powerplant.submit(it->reaction->getTask(NUClear::threading::ReactionTask::currentTask));
                                                }
                                                catch(...) {
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                // Reset our events
                                fd.revents = 0;
                            }
                        }
                        
                        // If our list is dirty
                        if(dirty) {
                            // Get the lock so we don't concurrently modify the list
                            std::lock_guard<std::mutex> lock(reactionMutex);
                            
                            // Clear our fds to be rebuilt
                            fds.clear();
                            
                            // Insert our notifyFd
                            fds.push_back(pollfd { notifyRecv, POLLIN, 0 });
                            
                            for (const auto& r : reactions) {
                                
                                // If we are the same fd, then add our interest set
                                if(r.fd == fds.back().fd) {
                                    fds.back().events |= r.events;
                                }
                                // Otherwise add a new one
                                else {
                                    fds.push_back(pollfd { r.fd, r.events, 0 });
                                }
                            }
                        }
                    }
                });
                
            }
            
        private:
            int notifyRecv;
            int notifySend;
            
            bool dirty;
            std::mutex reactionMutex;
            std::vector<pollfd> fds;
            std::vector<Task> reactions;
        };
    }
}

#endif
