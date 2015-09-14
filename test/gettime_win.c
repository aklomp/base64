#include <windows.h>
#include <time.h>

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 2
#endif

/* Number of 100 ns intervals from January 1, 1601 till January 1, 1970. */
const static unsigned long long unix_epoch = 116444736000000000;
#ifndef _TIMESPEC_DEFINED
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#endif

static ULARGE_INTEGER
xgetfiletime(void) {
	ULARGE_INTEGER current_time;
	FILETIME current_time_ft;

	/* Returns current time in UTC as a 64-bit value representing the number
	* of 100-nanosecond intervals since January 1, 1601 . */
	GetSystemTimePreciseAsFileTime(&current_time_ft);
	current_time.LowPart = current_time_ft.dwLowDateTime;
	current_time.HighPart = current_time_ft.dwHighDateTime;

	return current_time;
}

static int
clock_gettime(clock_t id, struct timespec* ts) {
	if (id == CLOCK_MONOTONIC) {
		static LARGE_INTEGER freq;
		LARGE_INTEGER count;
		long long int ns;

		if (!freq.QuadPart) {
			/* Number of counts per second. */
			QueryPerformanceFrequency(&freq);
		}
		/* Total number of counts from a starting point. */
		QueryPerformanceCounter(&count);

		/* Total nano seconds from a starting point. */
		ns = (long long int)((double)count.QuadPart / freq.QuadPart * 1000000000);

		ts->tv_sec = count.QuadPart / freq.QuadPart;
		ts->tv_nsec = ns % 1000000000;
	} else if (id == CLOCK_REALTIME) {
		ULARGE_INTEGER current_time = xgetfiletime();

		/* Time from Epoch to now. */
		ts->tv_sec = (current_time.QuadPart - unix_epoch) / 10000000;
		ts->tv_nsec = ((current_time.QuadPart - unix_epoch) %
			10000000) * 100;
	} else {
		return -1;
	}

	return 0;
}
