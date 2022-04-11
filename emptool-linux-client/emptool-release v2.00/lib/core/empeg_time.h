/* empeg_time.h
 *
 * Time handling
 *
 * (C) 2000 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.15 01-Apr-2003 18:52 rob:)
 */

#ifndef EMPEG_TIME_H
#define EMPEG_TIME_H 1

#ifndef CONFIG_H
#include "config.h"
#endif

#if !defined(WIN32)
#include <sys/time.h>
#else
// I kid you not.
#include <winsock2.h>
#endif

#include <time.h>

#ifndef TRACE_H
#include "trace.h"
#endif

class Time : public timeval
{
public:
    enum {
	ONE_SECOND		= 1000000
    };

    inline Time() {}
    
    inline explicit Time(int usec)
    {
	tv_sec = 0;
	tv_usec = usec;
	// handle common case of Time(constant)
	if(usec < 0 || usec >= ONE_SECOND)
	    Normalise();
    }

    inline Time(int sec, int usec)
    {
	tv_sec = sec;
	tv_usec = usec;
    }
    
    static inline Time FromMilliseconds(int ms)
    {
	ASSERT(ms >= 0);
	return Time(ms / 1000, ((ms % 1000) * 1000));
    }

#if !defined(WIN32)
    static inline Time Now()
    {
	Time t;
	VERIFY(gettimeofday(&t, NULL) == 0);
	return t;
    }
#else
    static inline Time Now()
    {
	// We need this in seconds, or at tenths of a second.
	SYSTEMTIME st;
	GetSystemTime(&st);
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	ULARGE_INTEGER ul;
	ul.LowPart = ft.dwLowDateTime;
	ul.HighPart = ft.dwHighDateTime;
	
	// 64-bit number - 100ns intervals since 1601.
	const INT64 intervalsPerMicrosecond = 10;
	const INT64 intervalsPerSecond = intervalsPerMicrosecond * 1000 * 1000;

	return Time(ul.QuadPart / intervalsPerSecond, (ul.QuadPart % intervalsPerSecond) / intervalsPerMicrosecond);
    }
#endif

#if DEBUG == 0
    inline void Clear() { tv_sec = tv_usec = 0; }
    
    inline Time &operator+=(int n)
    {
	tv_usec += n;
	Normalise();
	return *this;
    }

    inline Time &operator+=(const Time &t)
    {
	tv_sec += t.tv_sec;
	tv_usec += t.tv_usec;
	Normalise();
	return *this;
    }

    inline Time &operator-=(int n)
    {
	tv_usec -= n;
	Normalise();
	return *this;
    }
    
    inline Time &operator-=(const Time &t)
    {
	tv_sec -= t.tv_sec;
	tv_usec -= t.tv_usec;
	Normalise();
	return *this;
    }
	
#else
    void Clear();
    Time &operator+=(int n);
    Time &operator+=(const Time &t);
    Time &operator-=(int n);
    Time &operator-=(const Time &t);
#endif

    void Normalise();
    
#if !defined(WIN32)
    // Turn a struct timeval into a timespec. They are only different by
    //   - timespec uses longs, timeval uses ints.
    //   - timespec uses nanoseconds for the fractional part, timeval uses
    //     microseconds.
    // So, it's not too difficult to convert between them.
    //
    inline struct timespec GetAsTimeSpec() const
    {
	struct timespec ts;
	ts.tv_sec = tv_sec;
	ts.tv_nsec = tv_usec * 1000L;
	return ts;
    }
#endif

    inline int GetSeconds() const { return tv_sec; }

    inline int GetAsMilliseconds() const
    {
	ASSERT_EX(tv_sec <= 2147483 && tv_sec >= -2147483, "tv_sec %d\n", (int)tv_sec);
	return tv_sec * 1000 + tv_usec / 1000;
    }
    int GetAsMicroseconds() const
    {
        ASSERT_EX(tv_sec <= 2147 && tv_sec >= -2147, "tv_sec %d\n", (int)tv_sec);
	return tv_sec * 1000000 + tv_usec;
    }
    
    float GetAsFloatSeconds() const
    {
	return ((float) tv_sec) + ((float) tv_usec)*0.000001;
    }

#if DEBUG > 0
    void Dump(int trace, const char *s);
#else
    inline void Dump(int, const char *) { }
#endif
};

inline bool operator==(const Time &tv1, const Time &tv2)
{
    return (tv1.tv_sec == tv2.tv_sec)
	&& (tv1.tv_usec == tv2.tv_usec);
}

inline bool operator!=(const Time &tv1, const Time &tv2)
{
    return (tv1.tv_sec != tv2.tv_sec)
	|| (tv1.tv_usec != tv2.tv_usec);
}

inline Time operator+(const Time &t1, int us)
{
    Time t = t1;
    t += us;
    return t;
}

inline Time operator+(const Time &t1, const Time &t2)
{
    Time t = t1;
    t += t2;
    return t;
}

inline Time operator-(const Time &t1, const Time &t2)
{
    Time t = t1;
    t -= t2;
    return t;
}

inline Time operator-(const Time &t1, int us)
{
    Time t = t1;
    t -= us;
    return t;
}

bool operator<(const Time &tv1, const Time &tv2);
bool operator<=(const Time &tv1, const Time &tv2);
bool operator>(const Time &tv1, const Time &tv2);
bool operator>=(const Time &tv1, const Time &tv2);

#ifndef WIN32
inline unsigned int GetTickCount()
{
    return Time::Now().GetAsMilliseconds();
}
#define INFINITE ((unsigned)-1)
#endif

#endif
