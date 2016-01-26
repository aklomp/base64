#include <stdint.h>
#include <stddef.h>

#include "../include/libbase64.h"
#include "codecs.h"

#ifdef __SSSE3__
#include <tmmintrin.h>

#define CMPGT(s,n)	_mm_cmpgt_epi8((s), _mm_set1_epi8(n))
#define CMPEQ(s,n)	_mm_cmpeq_epi8((s), _mm_set1_epi8(n))
#define REPLACE(s,n)	_mm_and_si128((s), _mm_set1_epi8(n))

static inline __m128i
_mm_bswap_epi32 (const __m128i in)
{
	return _mm_shuffle_epi8(in, _mm_setr_epi8(
		 3,  2,  1,  0,
		 7,  6,  5,  4,
		11, 10,  9,  8,
		15, 14, 13, 12));
}

static inline __m128i
enc_reshuffle (__m128i in)
{
	// Reorder to 32-bit big-endian, duplicating the third byte in every
	// block of four. This copies the third byte to its final destination,
	// so we can include it later by just masking instead of shifting and
	// masking. The workset must be in big-endian, otherwise the shifted
	// bits do not carry over properly among adjacent bytes:
	in = _mm_shuffle_epi8(in, _mm_setr_epi8(
		 2,  2,  1,  0,
		 5,  5,  4,  3,
		 8,  8,  7,  6,
		11, 11, 10,  9));

	// Mask to pass through only the lower 6 bits of one byte:
	__m128i mask = _mm_set1_epi32(0x3F000000);

	// Shift bits by 2, mask in only the first byte:
	__m128i out = _mm_and_si128(_mm_srli_epi32(in, 2), mask);
	mask = _mm_srli_epi32(mask, 8);

	// Shift bits by 4, mask in only the second byte:
	out = _mm_or_si128(out, _mm_and_si128(_mm_srli_epi32(in, 4), mask));
	mask = _mm_srli_epi32(mask, 8);

	// Shift bits by 6, mask in only the third byte:
	out = _mm_or_si128(out, _mm_and_si128(_mm_srli_epi32(in, 6), mask));
	mask = _mm_srli_epi32(mask, 8);

	// No shift necessary for the fourth byte because we duplicated
	// the third byte to this position; just mask:
	out = _mm_or_si128(out, _mm_and_si128(in, mask));

	// Reorder to 32-bit little-endian and return:
	return _mm_bswap_epi32(out);
}

static inline __m128i
enc_translate (const __m128i in)
{
	// Translate values 0..63 to the Base64 alphabet. There are five sets:
	// #  From      To         Abs  Delta  Characters
	// 0  [0..25]   [65..90]   +65  +65    ABCDEFGHIJKLMNOPQRSTUVWXYZ
	// 1  [26..51]  [97..122]  +71   +6    abcdefghijklmnopqrstuvwxyz
	// 2  [52..61]  [48..57]    -4  -75    0123456789
	// 3  [62]      [43]       -19  -15    +
	// 4  [63]      [47]       -16   +3    /

	// Create cumulative masks for characters in sets [1,2,3,4], [2,3,4],
	// [3,4], and [4]:
	const __m128i mask1 = CMPGT(in, 25);
	const __m128i mask2 = CMPGT(in, 51);
	const __m128i mask3 = CMPGT(in, 61);
	const __m128i mask4 = CMPEQ(in, 63);

	// All characters are at least in cumulative set 0, so add 'A':
	__m128i out = _mm_add_epi8(in, _mm_set1_epi8(65));

	// For inputs which are also in any of the other cumulative sets,
	// add delta values against the previous set(s) to correct the shift:
	out = _mm_add_epi8(out, REPLACE(mask1,  6));
	out = _mm_sub_epi8(out, REPLACE(mask2, 75));
	out = _mm_sub_epi8(out, REPLACE(mask3, 15));
	out = _mm_add_epi8(out, REPLACE(mask4,  3));

	return out;
}

#endif	// __SSSE3__

BASE64_ENC_FUNCTION(ssse3)
{
#ifdef __SSSE3__
	#include "enc/head.c"
	#include "enc/ssse3.c"
	#include "enc/tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(ssse3)
{
#ifdef __SSSE3__
	#include "dec/head.c"
	#include "dec/ssse3.c"
	#include "dec/tail.c"
#else
	BASE64_DEC_STUB
#endif
}
