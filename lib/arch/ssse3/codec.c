#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#ifdef __SSSE3__
#include <tmmintrin.h>

#include "../sse2/compare_macros.h"

#include "dec_reshuffle.c"
#include "enc_translate.c"
#include "enc_reshuffle.c"

#endif	// __SSSE3__

BASE64_ENC_FUNCTION(ssse3)
{
#ifdef __SSSE3__
	#include "../generic/enc_head.c"
	#include "enc_loop.c"
	#include "../generic/enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(ssse3)
{
#ifdef __SSSE3__
	#include "../generic/dec_head.c"
	#include "dec_loop.c"
	#if BASE64_WORDSIZE == 32
		#include "../generic/32/dec_loop.c"
	#elif BASE64_WORDSIZE == 64
		#include "../generic/64/dec_loop.c"
	#endif
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
