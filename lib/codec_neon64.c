#if (defined(__ARM_NEON) && !defined(__ARM_NEON__))
#define __ARM_NEON__
#endif

#include <stddef.h>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "../include/libbase64.h"

extern const char base64_table_enc[];
extern const unsigned char base64_table_dec[];

#if (defined(__aarch64__) && defined(__ARM_NEON__))
/* With this transposed encoding table, we can use
 * a 64-byte lookup to do the encoding. Read the
 * table top to bottom, left to right. */
static const char *base64_table_enc_transposed =
{
	"AQgw"
	"BRhx"
	"CSiy"
	"DTjz"
	"EUk0"
	"FVl1"
	"GWm2"
	"HXn3"
	"IYo4"
	"JZp5"
	"Kaq6"
	"Lbr7"
	"Mcs8"
	"Ndt9"
	"Oeu+"
	"Pfv/"
};
#endif

/* Stride size is so large on these NEON 64-bit functions
 * (48 bytes encode, 64 bytes decode) that we inline the
 * uint64 codec to stay performant on smaller inputs. */

void
base64_stream_encode_neon64 (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
#if (defined(__aarch64__) && defined(__ARM_NEON__))
	uint8x16x4_t tbl_enc = vld4q_u8((uint8_t *)base64_table_enc_transposed);

	#include "enc/head.c"
	#include "enc/neon64.c"
	#include "enc/uint64.c"
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
base64_stream_decode_neon64 (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
#if (defined(__aarch64__) && defined(__ARM_NEON__))

	#include "dec/head.c"
	#include "dec/neon64.c"
	#include "dec/uint64.c"
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
