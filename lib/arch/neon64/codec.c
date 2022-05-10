#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../../../include/libbase64.h"
#include "../../tables/tables.h"
#include "../../codecs.h"
#include "config.h"
#include "../../env.h"

#ifdef __aarch64__
#  if (defined(__ARM_NEON__) || defined(__ARM_NEON)) && HAVE_NEON64
#    define BASE64_USE_NEON64
#  endif
#endif

#ifdef BASE64_USE_NEON64
#include <arm_neon.h>

static inline uint8x16x4_t
load_64byte_table (const uint8_t *p)
{
#if defined(__GNUC__) && !defined(__clang__)
	// As of October 2016, GCC does not support the 'vld1q_u8_x4()' intrinsic.
	uint8x16x4_t ret;
	ret.val[0] = vld1q_u8(p +  0);
	ret.val[1] = vld1q_u8(p + 16);
	ret.val[2] = vld1q_u8(p + 32);
	ret.val[3] = vld1q_u8(p + 48);
	return ret;
#else
	return vld1q_u8_x4(p);
#endif
}

#include "../generic/32/dec_loop.c"
#include "../generic/64/enc_loop.c"
#include "dec_loop.c"
#include "enc_reshuffle.c"
#include "enc_loop.c"

#endif	// BASE64_USE_NEON64

// Stride size is so large on these NEON 64-bit functions
// (48 bytes encode, 64 bytes decode) that we inline the
// uint64 codec to stay performant on smaller inputs.

BASE64_ENC_FUNCTION(neon64)
{
#ifdef BASE64_USE_NEON64
	#include "../generic/enc_head.c"
	enc_loop_neon64(&s, &slen, &o, &olen);
	enc_loop_generic_64(&s, &slen, &o, &olen);
	#include "../generic/enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(neon64)
{
#ifdef BASE64_USE_NEON64
	#include "../generic/dec_head.c"
	dec_loop_neon64(&s, &slen, &o, &olen);
	dec_loop_generic_32(&s, &slen, &o, &olen);
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
