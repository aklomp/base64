#define _POSIX_C_SOURCE 199309L		/* for clock_gettime() */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _WIN32
	#include <unistd.h>
#else
	#include <io.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#if defined(_WIN32)
	#include <windows.h>
	#include <BaseTsd.h>
	#include <Wincrypt.h>

	typedef SSIZE_T ssize_t;
#endif
#include "../include/libbase64.h"
#include "codec_supported.h"

#define INSIZE_MB  10
#define RANDOMDEV  "/dev/urandom"

#ifdef _WIN32
typedef unsigned int clockid_t;

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 2
#endif

/* Number of 100 ns intervals from January 1, 1601 till January 1, 1970. */
const static unsigned long long unix_epoch = 116444736000000000;
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
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
		ns = (double)count.QuadPart / freq.QuadPart * 1000000000;

		ts->tv_sec = count.QuadPart / freq.QuadPart;
		ts->tv_nsec = ns % 1000000000;
	}
	else if (id == CLOCK_REALTIME) {
		ULARGE_INTEGER current_time = xgetfiletime();

		/* Time from Epoch to now. */
		ts->tv_sec = (current_time.QuadPart - unix_epoch) / 10000000;
		ts->tv_nsec = ((current_time.QuadPart - unix_epoch) %
			10000000) * 100;
	}
	else {
		return -1;
	}

	return 0;
}

int 
crypt_rand(void* data, size_t length) {
	HCRYPTPROV hProvider = 0;

	if (!CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
		return 1;
	if (!CryptGenRandom(hProvider, length, data)) {
		CryptReleaseContext(hProvider, 0);
		return 1;
	}
	if (!CryptReleaseContext(hProvider, 0))
		return 1;
	return 0;
}

#endif /* _WIN32 */

struct buffers {
	char *reg;
	char *enc;
	size_t regsz;
	size_t encsz;
};

static int
get_random_data (struct buffers *b, char **errmsg)
{
#ifndef _WIN32
	int fd;
	ssize_t nread;
	size_t total_read = 0;
	/* Open random device for semi-random data: */
	if ((fd = open(RANDOMDEV, O_RDONLY)) < 0) {
		*errmsg = "Cannot open " RANDOMDEV "\n";
		return 0;
	}
	printf("Filling buffer with random data...\n");

	while (total_read < b->regsz) {
		if ((nread = read(fd, b->reg + total_read, b->regsz - total_read)) < 0) {
			*errmsg = "Read error\n";
			close(fd);
			return 0;
		}
		total_read += nread;
	}
	close(fd);
#else
	crypt_rand(b->reg, b->regsz);
#endif
	return 1;
}

static float
timediff_sec (struct timespec *start, struct timespec *end)
{
	return (end->tv_sec - start->tv_sec) + ((float)(end->tv_nsec - start->tv_nsec)) / 1e9f;
}

static void
codec_bench_enc (struct buffers *b, const char *name, unsigned int flags)
{
	int i;
	float timediff, fastest = -1.0f;
	struct timespec start, end;

	/* Choose fastest of ten runs: */
	for (i = 0; i < 10; i++) {
		clock_gettime(CLOCK_REALTIME, &start);
		base64_encode(b->reg, b->regsz, b->enc, &b->encsz, flags);
		clock_gettime(CLOCK_REALTIME, &end);
		timediff = timediff_sec(&start, &end);
		if (fastest < 0.0f || timediff < fastest) {
			fastest = timediff;
		}
	}
	printf("%s\tencode\t%.02f MB/sec\n", name, (((float)b->regsz) / 1024.0f / 1024.0f) / fastest);
}

static void
codec_bench_dec (struct buffers *b, const char *name, unsigned int flags)
{
	int i;
	float timediff, fastest = -1.0f;
	struct timespec start, end;

	/* Choose fastest of ten runs: */
	for (i = 0; i < 10; i++) {
		clock_gettime(CLOCK_REALTIME, &start);
		base64_decode(b->enc, b->encsz, b->reg, &b->regsz, flags);
		clock_gettime(CLOCK_REALTIME, &end);
		timediff = timediff_sec(&start, &end);
		if (fastest < 0.0f || timediff < fastest) {
			fastest = timediff;
		}
	}
	printf("%s\tdecode\t%.02f MB/sec\n", name, (((float)b->encsz) / 1024.0f / 1024.0f) / fastest);
}

static void
codec_bench (struct buffers *b, const char *name, unsigned int flags)
{
	codec_bench_enc(b, name, flags);
	codec_bench_dec(b, name, flags);
}

int
main ()
{
	int i;
	char *errmsg;
	struct buffers b;

	b.regsz = 1024 * 1024 * INSIZE_MB;
	b.encsz = 1024 * 1024 * ((INSIZE_MB * 5) / 3);

	/* Allocate space for megabytes of random data: */
	if ((b.reg = malloc(b.regsz)) == NULL) {
		errmsg = "Out of memory\n";
		goto err0;
	}
	/* Allocate space for encoded output: */
	if ((b.enc = malloc(b.encsz)) == NULL) {
		errmsg = "Out of memory\n";
		goto err1;
	}
	/* Fill buffer with random data: */
	if (get_random_data(&b, &errmsg) == 0) {
		goto err2;
	}
	/* Loop over all codecs: */
	for (i = 0; codecs[i]; i++) {
		if (codec_supported(1 << i)) {
			codec_bench(&b, codecs[i], 1 << i);
		}
	}
	/* Free memory: */
	free(b.enc);
	free(b.reg);
	return 0;

err2:	free(b.enc);
err1:	free(b.reg);
err0:	fputs(errmsg, stderr);
	return 1;
}
