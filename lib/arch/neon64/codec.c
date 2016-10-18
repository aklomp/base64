#if (defined(__ARM_NEON) && !defined(__ARM_NEON__))
#define __ARM_NEON__
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#if (defined(__aarch64__) && defined(__ARM_NEON__))

#define CMPGT(s,n)	vcgtq_u8((s), vdupq_n_u8(n))
#define CMPEQ(s,n)	vceqq_u8((s), vdupq_n_u8(n))
#define REPLACE(s,n)	vandq_u8((s), vdupq_n_u8(n))
#define RANGE(s,a,b)	vandq_u8(vcgeq_u8((s), vdupq_n_u8(a)), vcleq_u8((s), vdupq_n_u8(b)))

// With this transposed encoding table, we can use
// a 64-byte lookup to do the encoding.
// Read the table top to bottom, left to right.
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

// Stride size is so large on these NEON 64-bit functions
// (48 bytes encode, 64 bytes decode) that we inline the
// uint64 codec to stay performant on smaller inputs.

BASE64_ENC_FUNCTION(neon64)
{
#if (defined(__aarch64__) && defined(__ARM_NEON__))
	uint8x16x4_t tbl_enc = vld4q_u8((uint8_t const*)base64_table_enc_transposed);

	#include "../generic/enc_head.c"
	#include "enc_loop.c"
	#include "../generic/64/enc_loop.c"
	#include "../generic/enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(neon64)
{
#if (defined(__aarch64__) && defined(__ARM_NEON__))

	#include "../generic/dec_head.c"
	#include "../neon32/dec_loop.c"
	#include "../generic/64/dec_loop.c"
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
