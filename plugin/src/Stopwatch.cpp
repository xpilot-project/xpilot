/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Tammo Ippen
 * https://github.com/tammoippen/timer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cassert>

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "Stopwatch.h"

namespace xpilot
{
    std::ostream&
        operator<<(std::ostream& os, const Stopwatch& stopwatch)
    {
        stopwatch.print("", Stopwatch::SECONDS, os);
        return os;
    }

    Stopwatch::Stopwatch()
    {
        reset();
    }

    void Stopwatch::start()
    {
        if (!isRunning())
        {
            _prev_elapsed += _end - _beg;  // store prev. time, if we resume
            _end = _beg = get_timestamp(); // invariant: _end >= _beg
            _running = true;               // we start running
        }
    }

    void Stopwatch::stop()
    {
        if (isRunning())
        {
            _end = get_timestamp(); // invariant: _end >= _beg
            _running = false;       // we stopped running
        }
    }

    bool Stopwatch::isRunning() const
    {
        return _running;
    }

    double Stopwatch::elapsed(timeunit_t timeunit) const
    {
        assert(correct_timeunit(timeunit));
        return 1.0 * elapsed_timestamp() / timeunit;
    }

    Stopwatch::timestamp_t Stopwatch::elapsed_timestamp() const
    {
        if (isRunning())
        {
            // get intermediate elapsed time; do not change _end, to be const
            return get_timestamp() - _beg + _prev_elapsed;
        }
        else
        {
            // stopped before, get time of current measurment + last measurments
            return _end - _beg + _prev_elapsed;
        }
    }

    void Stopwatch::reset()
    {
        _beg = 0; // invariant: _end >= _beg
        _end = 0;
        _prev_elapsed = 0; // erase all prev. measurments
        _running = false;  // of course not running.
    }

    void Stopwatch::print(const char* msg, timeunit_t timeunit, std::ostream& os) const
    {
        assert(correct_timeunit(timeunit));
        double e = elapsed(timeunit);
        os << msg << e;
        switch (timeunit)
        {
        case MICROSEC:
            os << " microsec.";
            break;
        case MILLISEC:
            os << " millisec.";
            break;
        case SECONDS:
            os << " sec.";
            break;
        case MINUTES:
            os << " min.";
            break;
        case HOURS:
            os << " h.";
            break;
        case DAYS:
            os << " days.";
            break;
        }
    }

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64 

    // MSVC defines this in winsock2.h!?
    typedef struct timeval {
        long tv_sec;
        long tv_usec;
    } timeval;

    int gettimeofday(struct timeval* tp, struct timezone* tzp)
    {
        // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
        // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
        // until 00:00:00 January 1, 1970 
        static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

        SYSTEMTIME  system_time;
        FILETIME    file_time;
        uint64_t    time;

        GetSystemTime(&system_time);
        SystemTimeToFileTime(&system_time, &file_time);
        time = ((uint64_t)file_time.dwLowDateTime);
        time += ((uint64_t)file_time.dwHighDateTime) << 32;

        tp->tv_sec = (long)((time - EPOCH) / 10000000L);
        tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
        return 0;
    }

#endif

    Stopwatch::timestamp_t Stopwatch::get_timestamp()
    {
        struct timeval now;
        gettimeofday(&now, (struct timezone*) 0);
        return (Stopwatch::timestamp_t) now.tv_usec
            + (Stopwatch::timestamp_t) now.tv_sec * Stopwatch::SECONDS;
    }

}
