/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_UTIL_WINDOWS_INCLUDES_HPP
#define NUCLEAR_UTIL_WINDOWS_INCLUDES_HPP

// Because windows is SUUUUPER dumb and if you include headers in the wrong order
// Nothing at all works, also if you don't define things in the right order nothing works
// It's a terrible pile of garbage
// So we have this header to make sure everything is in the correct order
#ifdef _WIN32

// Windows has a dumb min/max macro that breaks stuff
#define NOMINMAX

// Without this winsock just doesn't have half the typedefs
#define INCL_WINSOCK_API_TYPEDEFS 1

// Winsock must be declared before Windows.h or it won't work
#include <WinSock2.h>

#include <Ws2ipdef.h>
#include <Ws2tcpip.h>

#include <Mstcpip.h>
#include <Mswsock.h>

#include <Iphlpapi.h>

// This little thingy makes windows link to the winsock library
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "IPHLPAPI.lib")

// Include windows.h mega header... no wonder windows compiles so slowly
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Whoever thought this was a good idea was a terrible person
#undef ERROR

#endif  // _WIN32

#endif  // NUCLEAR_UTIL_WINDOWS_INCLUDES_HPP
