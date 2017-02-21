#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#ifdef __SSE4_1__
#include <smmintrin.h>

#include "../sse2/compare_macros.h"

#include "../ssse3/dec_reshuffle.c"
#include "../ssse3/enc_translate.c"
#include "../ssse3/enc_reshuffle.c"

#endif	// __SSE4_1__

BASE64_ENC_FUNCTION(sse41)
{
#ifdef __SSE4_1__
	#include "../generic/enc_head.c"
	#include "../ssse3/enc_loop.c"
	#include "../generic/enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(sse41)
{
#ifdef __SSE4_1__
	#include "../generic/dec_head.c"
	#include "../ssse3/dec_loop.c"
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
