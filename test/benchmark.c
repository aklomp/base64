#define _POSIX_C_SOURCE 199309L		// for clock_gettime()

#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "../include/libbase64.h"
#include "codec_supported.h"

#define INSIZE_MB  10
#define RANDOMDEV  "/dev/urandom"

struct buffers {
	char *reg;
	char *enc;
	size_t regsz;
	size_t encsz;
};

static inline float
bytes_to_mb (size_t bytes)
{
	return bytes / 1024.0f / 1024.0f;
}

static bool
get_random_data (struct buffers *b, char **errmsg)
{
	int fd;
	ssize_t nread;
	size_t total_read = 0;

	// Open random device for semi-random data:
	if ((fd = open(RANDOMDEV, O_RDONLY)) < 0) {
		*errmsg = "Cannot open " RANDOMDEV;
		return false;
	}

	printf("Filling buffer with %.1f MB of random data...\n", bytes_to_mb(b->regsz));

	while (total_read < b->regsz) {
		if ((nread = read(fd, b->reg + total_read, b->regsz - total_read)) < 0) {
			*errmsg = "Read error";
			close(fd);
			return false;
		}
		total_read += nread;
	}
	close(fd);
	return true;
}

static float
timediff_sec (struct timespec *start, struct timespec *end)
{
	return (end->tv_sec - start->tv_sec) + ((float)(end->tv_nsec - start->tv_nsec)) / 1e9f;
}

static void
codec_bench_enc (struct buffers *b, const char *name, unsigned int flags)
{
	float timediff, fastest = -1.0f;
	struct timespec start, end;

	// Choose fastest of ten runs:
	for (int i = 0; i < 10; i++) {
		clock_gettime(CLOCK_REALTIME, &start);
		base64_encode(b->reg, b->regsz, b->enc, &b->encsz, flags);
		clock_gettime(CLOCK_REALTIME, &end);
		timediff = timediff_sec(&start, &end);
		if (fastest < 0.0f || timediff < fastest)
			fastest = timediff;
	}

	printf("%s\tencode\t%.02f MB/sec\n", name, bytes_to_mb(b->regsz) / fastest);
}

static void
codec_bench_dec (struct buffers *b, const char *name, unsigned int flags)
{
	float timediff, fastest = -1.0f;
	struct timespec start, end;

	// Choose fastest of ten runs:
	for (int i = 0; i < 10; i++) {
		clock_gettime(CLOCK_REALTIME, &start);
		base64_decode(b->enc, b->encsz, b->reg, &b->regsz, flags);
		clock_gettime(CLOCK_REALTIME, &end);
		timediff = timediff_sec(&start, &end);
		if (fastest < 0.0f || timediff < fastest)
			fastest = timediff;
	}

	printf("%s\tdecode\t%.02f MB/sec\n", name, bytes_to_mb(b->encsz) / fastest);
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
	int ret = 0;
	char *errmsg = NULL;
	struct buffers b;

	b.regsz = 1024 * 1024 * INSIZE_MB;
	b.encsz = 1024 * 1024 * ((INSIZE_MB * 5) / 3);

	// Allocate space for megabytes of random data:
	if ((b.reg = malloc(b.regsz)) == NULL) {
		errmsg = "Out of memory";
		ret = 1;
		goto err0;
	}

	// Allocate space for encoded output:
	if ((b.enc = malloc(b.encsz)) == NULL) {
		errmsg = "Out of memory";
		ret = 1;
		goto err1;
	}

	// Fill buffer with random data:
	if (get_random_data(&b, &errmsg) == false) {
		ret = 1;
		goto err2;
	}

	// Loop over all codecs:
	for (size_t i = 0; codecs[i]; i++)
		if (codec_supported(1 << i))
			codec_bench(&b, codecs[i], 1 << i);

	// Free memory:
err2:	free(b.enc);
err1:	free(b.reg);
err0:	if (errmsg)
		fputs(errmsg, stderr);

	return ret;
}
