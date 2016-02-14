#if (defined(__ARM_NEON) && !defined(__ARM_NEON__))
#define __ARM_NEON__
#endif

#include <stdint.h>
#include <stddef.h>
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "../include/libbase64.h"
#include "codecs.h"

#if (defined(__aarch64__) && defined(__ARM_NEON__))
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
	uint8x16x4_t tbl_enc = vld4q_u8(base64_table_enc_transposed);

	#include "enc/head.c"
	#include "enc/neon64.c"
	#include "enc/uint64.c"
	#include "enc/tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(neon64)
{
#if (defined(__aarch64__) && defined(__ARM_NEON__))

	#include "dec/head.c"
	#include "dec/neon.c"
	#include "dec/uint64.c"
	#include "dec/tail.c"
#else
	BASE64_DEC_STUB
#endif
}
