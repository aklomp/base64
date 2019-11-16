#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#ifdef __arm__
#  if (defined(__ARM_NEON__) || defined(__ARM_NEON)) && HAVE_NEON32
#    define BASE64_USE_NEON32
#  endif
#endif

#ifdef BASE64_USE_NEON32
#include <arm_neon.h>

#include "enc_reshuffle.c"
#include "enc_translate.c"

#endif	// BASE64_USE_NEON32

// Stride size is so large on these NEON 32-bit functions
// (48 bytes encode, 32 bytes decode) that we inline the
// uint32 codec to stay performant on smaller inputs.

BASE64_ENC_FUNCTION(neon32)
{
#ifdef BASE64_USE_NEON32
	#include "../generic/enc_head.c"
	#include "enc_loop.c"
	#include "../generic/32/enc_loop.c"
	#include "../generic/enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(neon32)
{
#ifdef BASE64_USE_NEON32
	#include "../generic/dec_head.c"
	#include "dec_loop.c"
	#include "../generic/32/dec_loop.c"
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
