#include <stddef.h>
#include <stdint.h>

#include "../include/libbase64.h"

extern const char base64_table_enc[];
extern const unsigned char base64_table_dec[];

void
base64_stream_encode_plain (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
	#include "enc/head.c"
#if __WORDSIZE == 32
	#include "enc/uint32.c"
#elif __WORDSIZE == 64
	#include "enc/uint64.c"
#endif
	#include "enc/tail.c"
}

int
base64_stream_decode_plain (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
	#include "dec/head.c"
#if __WORDSIZE == 32
	#include "dec/uint32.c"
#elif __WORDSIZE == 64
	#include "dec/uint64.c"
#endif
	#include "dec/tail.c"
}
