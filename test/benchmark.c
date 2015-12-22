#define _POSIX_C_SOURCE 199309L		/* for clock_gettime() */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#if defined(_WIN32) && !defined(_POSIX_CPUTIME)
#include "gettime_win.c"
#endif

#include "../include/libbase64.h"
#include "codec_supported.h"

#define INSIZE_MB  10

struct buffers {
	char *reg;
	char *enc;
	size_t regsz;
	size_t encsz;
};
#ifndef _WIN32
#include "rand_unix.c"
#else
#include "rand_win.c"
#endif

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
