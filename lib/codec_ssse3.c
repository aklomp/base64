#include <stddef.h>
#ifdef __SSSE3__
#include <tmmintrin.h>
#endif

#include "../include/libbase64.h"

extern const char base64_table_enc[];
extern const unsigned char base64_table_dec[];

void
base64_stream_encode_ssse3 (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
#ifdef __SSSE3__
	#include "enc/head.c"
	#include "enc/ssse3.c"
	#include "enc/tail.c"
#else
	(void)state;
	(void)src;
	(void)srclen;
	(void)out;

	*outlen = 0;
#endif
}

int
base64_stream_decode_ssse3 (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
#ifdef __SSSE3__
	#include "dec/head.c"
	#include "dec/ssse3.c"
	#include "dec/tail.c"
#else
	(void)state;
	(void)src;
	(void)srclen;
	(void)out;
	(void)outlen;

	return -1;
#endif
}
