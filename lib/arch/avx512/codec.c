#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../tables/tables.h"
#include "../../codecs.h"
#include "config.h"
#include "../../env.h"

#if HAVE_AVX512
#include <immintrin.h>

#include "enc_loop.c"
#include "chromiumbase64.c"
#include "../avx2/dec_reshuffle.c"
#include "../avx2/dec_loop.c"

#endif	// HAVE_AVX512

BASE64_ENC_FUNCTION(avx512)
{
#if HAVE_AVX512
	enc_loop_avx512(src, srclen, out, outlen);
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(avx512)
{
// avx512 decode is not implemented yet, reuse avx2 version
#if HAVE_AVX512
	#include "../generic/dec_head.c"
	dec_loop_avx2(&s, &slen, &o, &olen);
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}