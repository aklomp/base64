#include <stddef.h>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "../include/libbase64.h"

extern const char base64_table_enc[];
extern const unsigned char base64_table_dec[];

void
base64_stream_encode_neon (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
#ifdef __ARM_NEON__
	#include "enc/head.c"
	#include "enc/neon.c"
	#include "enc/tail.c"
#else
	(void)state;
	(void)src;
	(void)srclen;
	(void)out;
	(void)outlen;
#endif
}

int
base64_stream_decode_neon (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
#ifdef __ARM_NEON__
	#include "dec/head.c"
	#include "dec/neon.c"
	#include "dec/tail.c"
#else
	(void)state;
	(void)src;
	(void)srclen;
	(void)out;
	(void)outlen;

	return 0;
#endif
}
