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

#ifndef NUCLEAR_UTIL_NETWORK_RWLOOP_H
#define NUCLEAR_UTIL_NETWORK_RWLOOP_H

namespace NUClear {
    namespace util {
        namespace network {
            
            inline ssize_t readAll(int fd, void* data, size_t len) {
                ssize_t done = 0;
                char* d = reinterpret_cast<char*>(data);
                
                while (done < ssize_t(len)) {
                    ssize_t r = ::read(fd, d + done, len - size_t(done));
                    
                    if(r == -1) {
                        return -1;
                    }
                    else if(r == 0) {
                        return done;
                    }
                    done += r;
                }
                
                return done;
            }
            
            inline ssize_t writeAll(int fd, const void* data, size_t len) {
                ssize_t done = 0;
                const char* d = reinterpret_cast<const char*>(data);
                
                while (done < ssize_t(len)) {
                    ssize_t r = ::write(fd, d + done, len - size_t(done));
                    
                    if(r == -1) {
                        return -1;
                    }
                    if(r == 0) {
                        return done;
                    }
                    done += r;
                }
                
                return done;
            }
        }
    }
}

#endif
