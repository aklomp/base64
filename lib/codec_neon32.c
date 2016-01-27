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

#if (defined(__arm__) && defined(__ARM_NEON__))
// With this transposed encoding table, we can use
// two 32-byte lookups to do the encoding.
// Read the tables top to bottom, left to right.
static const uint8_t *base64_table_enc_transposed[2] =
{
	"AIQY"
	"BJRZ"
	"CKSa"
	"DLTb"
	"EMUc"
	"FNVd"
	"GOWe"
	"HPXf"
,
	"gow4"
	"hpx5"
	"iqy6"
	"jrz7"
	"ks08"
	"lt19"
	"mu2+"
	"nv3/"
};
#endif

// Stride size is so large on these NEON 32-bit functions
// (48 bytes encode, 32 bytes decode) that we inline the
// uint32 codec to stay performant on smaller inputs.

BASE64_ENC_FUNCTION(neon32)
{
#if (defined(__arm__) && defined(__ARM_NEON__))
	uint8x8x4_t tbl_enc_lo = vld4_u8(base64_table_enc_transposed[0]);
	uint8x8x4_t tbl_enc_hi = vld4_u8(base64_table_enc_transposed[1]);

	#include "enc/head.c"
	#include "enc/neon32.c"
	#include "enc/uint32.c"
	#include "enc/tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(neon32)
{
#if (defined(__arm__) && defined(__ARM_NEON__))
	#include "dec/head.c"
	#include "dec/neon32.c"
	#include "dec/uint32.c"
	#include "dec/tail.c"
#else
	BASE64_DEC_STUB
#endif
}
