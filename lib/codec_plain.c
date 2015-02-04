#include <stddef.h>

#include "../include/libbase64.h"

extern const char base64_table_enc[];
extern const unsigned char base64_table_dec[];

void
base64_stream_encode_plain (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
	#include "enc/head.c"
	#include "enc/tail.c"
}

int
base64_stream_decode_plain (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
	#include "dec/head.c"
	#include "dec/tail.c"
}
