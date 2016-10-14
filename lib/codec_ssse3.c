#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../include/libbase64.h"
#include "codecs.h"

#ifdef __SSSE3__
#include <tmmintrin.h>

#define CMPGT(s,n)	_mm_cmpgt_epi8((s), _mm_set1_epi8(n))
#define CMPEQ(s,n)	_mm_cmpeq_epi8((s), _mm_set1_epi8(n))
#define REPLACE(s,n)	_mm_and_si128((s), _mm_set1_epi8(n))
#define RANGE(s,a,b)	_mm_andnot_si128(CMPGT((s), (b)), CMPGT((s), (a) - 1))

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

static inline __m128i
enc_translate (const __m128i in)
{
	// LUT contains Absolute offset for all ranges:
	const __m128i lut = _mm_setr_epi8(65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0);

	// Translate values 0..63 to the Base64 alphabet. There are five sets:
	// #  From      To         Abs    Index  Characters
	// 0  [0..25]   [65..90]   +65        0  ABCDEFGHIJKLMNOPQRSTUVWXYZ
	// 1  [26..51]  [97..122]  +71        1  abcdefghijklmnopqrstuvwxyz
	// 2  [52..61]  [48..57]    -4  [2..11]  0123456789
	// 3  [62]      [43]       -19       12  +
	// 4  [63]      [47]       -16       13  /

	// Create LUT indices from input:
	// the index for range #0 is right, others are 1 less than expected:
	__m128i indices = _mm_subs_epu8(in, _mm_set1_epi8(51));

	// mask is 0xFF (-1) for range #[1..4] and 0x00 for range #0:
	__m128i mask = CMPGT(in, 25);

	// substract -1, so add 1 to indices for range #[1..4], All indices are now correct:
	indices = _mm_sub_epi8(indices, mask);

	// Add offsets to input values:
	__m128i out = _mm_add_epi8(in, _mm_shuffle_epi8(lut, indices));

	return out;
}

static inline __m128i
dec_reshuffle (__m128i in)
{
	// Mask in a single byte per shift:
	const __m128i maskB2 = _mm_set1_epi32(0x003F0000);
	const __m128i maskB1 = _mm_set1_epi32(0x00003F00);

	// Pack bytes together:
	__m128i out = _mm_srli_epi32(in, 16);

	out = _mm_or_si128(out, _mm_srli_epi32(_mm_and_si128(in, maskB2), 2));

	out = _mm_or_si128(out, _mm_slli_epi32(_mm_and_si128(in, maskB1), 12));

	out = _mm_or_si128(out, _mm_slli_epi32(in, 26));

	// Reshuffle and repack into 12-byte output format:
	return _mm_shuffle_epi8(out, _mm_setr_epi8(
		 3,  2,  1,
		 7,  6,  5,
		11, 10,  9,
		15, 14, 13,
		-1, -1, -1, -1));
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
