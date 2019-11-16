#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#if HAVE_AVX2
#include <immintrin.h>

#include "dec_reshuffle.c"
#include "enc_translate.c"
#include "enc_reshuffle.c"

#endif	// HAVE_AVX2

BASE64_ENC_FUNCTION(avx2)
{
#if HAVE_AVX2
	#include "../generic/enc_head.c"
	#include "enc_loop.c"
	#include "../generic/enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(avx2)
{
#if HAVE_AVX2
	#include "../generic/dec_head.c"
	#include "dec_loop.c"
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
