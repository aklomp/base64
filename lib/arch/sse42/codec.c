#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#ifdef __SSE4_2__
#include <nmmintrin.h>

#include "../sse2/compare_macros.h"

#include "../ssse3/dec_reshuffle.c"
#include "../ssse3/enc_translate.c"
#include "../ssse3/enc_reshuffle.c"

#endif	// __SSE4_2__

BASE64_ENC_FUNCTION(sse42)
{
#ifdef __SSE4_2__
	#include "../generic/enc_head.c"
	#include "../ssse3/enc_loop.c"
	#include "../generic/enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(sse42)
{
#ifdef __SSE4_2__
	#include "../generic/dec_head.c"
	#include "../ssse3/dec_loop.c"
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
