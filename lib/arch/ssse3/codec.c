#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#ifdef __SSSE3__
#include <tmmintrin.h>

#include "../sse2/compare_macros.h"

#include "_mm_bswap_epi32.c"
#include "dec_reshuffle.c"
#include "enc_translate.c"

static inline __m128i
enc_reshuffle (__m128i in)
{
	// Slice into 32-bit chunks and operate on all chunks in parallel.
	// All processing is done within the 32-bit chunk. First, shuffle:
	// before: [eeeeeeff|ccdddddd|bbbbcccc|aaaaaabb]
	// after:  [00000000|aaaaaabb|bbbbcccc|ccdddddd]
	in = _mm_shuffle_epi8(in, _mm_set_epi8(
		-1, 9, 10, 11,
		-1, 6,  7,  8,
		-1, 3,  4,  5,
		-1, 0,  1,  2));

	// cd      = [00000000|00000000|0000cccc|ccdddddd]
	const __m128i cd = _mm_and_si128(in, _mm_set1_epi32(0x00000FFF));

	// ab      = [0000aaaa|aabbbbbb|00000000|00000000]
	const __m128i ab = _mm_and_si128(_mm_slli_epi32(in, 4), _mm_set1_epi32(0x0FFF0000));

	// merged  = [0000aaaa|aabbbbbb|0000cccc|ccdddddd]
	const __m128i merged = _mm_or_si128(ab, cd);

	// bd      = [00000000|00bbbbbb|00000000|00dddddd]
	const __m128i bd = _mm_and_si128(merged, _mm_set1_epi32(0x003F003F));

	// ac      = [00aaaaaa|00000000|00cccccc|00000000]
	const __m128i ac = _mm_and_si128(_mm_slli_epi32(merged, 2), _mm_set1_epi32(0x3F003F00));

	// indices = [00aaaaaa|00bbbbbb|00cccccc|00dddddd]
	const __m128i indices = _mm_or_si128(ac, bd);

	// return  = [00dddddd|00cccccc|00bbbbbb|00aaaaaa]
	return _mm_bswap_epi32(indices);
}

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
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
