/*
 * The MIT License(MIT)
 * Copyright(c) 2016 Tammo Ippen
 * https://github.com/tammoippen/timer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this softwareand associated documentation files(the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions :
 *
 * The above copyright noticeand this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
*/

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <iostream>
#include <stdint.h>

namespace xpilot
{
    class Stopwatch
    {
    public:
        typedef uint64_t timestamp_t;
        typedef uint64_t timeunit_t;

        enum
        {
            MICROSEC = (timeunit_t)1,
            MILLISEC = MICROSEC * 1000,
            SECONDS = MILLISEC * 1000,
            MINUTES = SECONDS * 60,
            HOURS = MINUTES * 60,
            DAYS = HOURS * 24
        };

        static bool correct_timeunit(timeunit_t t);

        /**
         * Creates a stopwatch that is not running.
         */
        Stopwatch();

        /**
         * Starts or resumes the stopwatch, if it is not running already.
         */
        void start();

        /**
         * Stops the stopwatch, if it is not stopped already.
         */
        void stop();

        /**
         * Returns, whether the stopwatch is running.
         */
        bool isRunning() const;

        /**
         * Returns the time elapsed between the start and stop of the
         * stopwatch. If it is running, it returns the time from start
         * until now. If the stopwatch is run previously, the previous
         * runtime is added. If you want only the last measurment, you
         * have to reset the timer, before stating the measurment.
         * Does not change the running state.
         */
        double elapsed(timeunit_t timeunit = SECONDS) const;

        /**
         * Returns the time elapsed between the start and stop of the
         * stopwatch. If it is running, it returns the time from start
         * until now. If the stopwatch is run previously, the previous
         * runtime is added. If you want only the last measurment, you
         * have to reset the timer, before stating the measurment.
         * Does not change the running state.
         * In contrast to Stopwatch::elapsed(), only the timestamp is returned,
         * that is the number if microseconds as an integer.
         */
        timestamp_t elapsed_timestamp() const;

        /**
         * Resets the stopwatch.
         */
        void reset();

        /**
         * This method prints out the currently elapsed time.
         */
        void print(const char* msg = "", timeunit_t timeunit = SECONDS, std::ostream& os = std::cout) const;

        /**
         * Convenient method for writing time in seconds
         * to some ostream.
         */
        friend std::ostream& operator<<(std::ostream& os, const Stopwatch& stopwatch);

    private:
        timestamp_t _beg, _end;
        uint64_t _prev_elapsed;
        bool _running;

        /**
         * Returns current time in microseconds since EPOCH.
         */
        static timestamp_t get_timestamp();
    };

    inline bool Stopwatch::correct_timeunit(timeunit_t t)
    {
        return t == MICROSEC || t == MILLISEC || t == SECONDS || t == MINUTES || t == HOURS || t == DAYS;
    }
}

#endif /* STOPWATCH_H */