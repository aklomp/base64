#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#ifdef __AVX2__
#include <immintrin.h>

#define CMPGT(s,n)	_mm256_cmpgt_epi8((s), _mm256_set1_epi8(n))
#define CMPEQ(s,n)	_mm256_cmpeq_epi8((s), _mm256_set1_epi8(n))
#define REPLACE(s,n)	_mm256_and_si256((s), _mm256_set1_epi8(n))
#define RANGE(s,a,b)	_mm256_andnot_si256(CMPGT((s), (b)), CMPGT((s), (a) - 1))

static inline __m256i
_mm256_bswap_epi32 (const __m256i in)
{
	// _mm256_shuffle_epi8() works on two 128-bit lanes separately:
	return _mm256_shuffle_epi8(in, _mm256_setr_epi8(
		 3,  2,  1,  0,
		 7,  6,  5,  4,
		11, 10,  9,  8,
		15, 14, 13, 12,
		 3,  2,  1,  0,
		 7,  6,  5,  4,
		11, 10,  9,  8,
		15, 14, 13, 12));
}

static inline __m256i
enc_reshuffle (__m256i in)
{
	// Spread out 32-bit words over both halves of the input register:
	in = _mm256_permutevar8x32_epi32(in, _mm256_setr_epi32(
		0, 1, 2, -1,
		3, 4, 5, -1));

	// Slice into 32-bit chunks and operate on all chunks in parallel.
	// All processing is done within the 32-bit chunk. First, shuffle:
	// before: [eeeeeeff|ccdddddd|bbbbcccc|aaaaaabb]
	// after:  [00000000|aaaaaabb|bbbbcccc|ccdddddd]
	in = _mm256_shuffle_epi8(in, _mm256_set_epi8(
		-1, 9, 10, 11,
		-1, 6,  7,  8,
		-1, 3,  4,  5,
		-1, 0,  1,  2,
		-1, 9, 10, 11,
		-1, 6,  7,  8,
		-1, 3,  4,  5,
		-1, 0,  1,  2));

	// merged  = [0000aaaa|aabbbbbb|bbbbcccc|ccdddddd]
	const __m256i merged = _mm256_blend_epi16(_mm256_slli_epi32(in, 4), in, 0x55);

	// bd      = [00000000|00bbbbbb|00000000|00dddddd]
	const __m256i bd = _mm256_and_si256(merged, _mm256_set1_epi32(0x003F003F));

	// ac      = [00aaaaaa|00000000|00cccccc|00000000]
	const __m256i ac = _mm256_and_si256(_mm256_slli_epi32(merged, 2), _mm256_set1_epi32(0x3F003F00));

	// indices = [00aaaaaa|00bbbbbb|00cccccc|00dddddd]
	const __m256i indices = _mm256_or_si256(ac, bd);

	// return  = [00dddddd|00cccccc|00bbbbbb|00aaaaaa]
	return _mm256_bswap_epi32(indices);
}

static inline __m256i
enc_translate (const __m256i in)
{
	// LUT contains Absolute offset for all ranges:
	const __m256i lut = _mm256_setr_epi8(65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0,
	                                     65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0);
	// Translate values 0..63 to the Base64 alphabet. There are five sets:
	// #  From      To         Abs    Index  Characters
	// 0  [0..25]   [65..90]   +65        0  ABCDEFGHIJKLMNOPQRSTUVWXYZ
	// 1  [26..51]  [97..122]  +71        1  abcdefghijklmnopqrstuvwxyz
	// 2  [52..61]  [48..57]    -4  [2..11]  0123456789
	// 3  [62]      [43]       -19       12  +
	// 4  [63]      [47]       -16       13  /

	// Create LUT indices from input:
	// the index for range #0 is right, others are 1 less than expected:
	__m256i indices = _mm256_subs_epu8(in, _mm256_set1_epi8(51));

	// mask is 0xFF (-1) for range #[1..4] and 0x00 for range #0:
	__m256i mask = CMPGT(in, 25);

	// substract -1, so add 1 to indices for range #[1..4], All indices are now correct:
	indices = _mm256_sub_epi8(indices, mask);

	// Add offsets to input values:
	__m256i out = _mm256_add_epi8(in, _mm256_shuffle_epi8(lut, indices));

	return out;
}

static inline __m256i
dec_reshuffle (__m256i in)
{
	// Mask in a single byte per shift:
	const __m256i maskB2 = _mm256_set1_epi32(0x003F0000);
	const __m256i maskB1 = _mm256_set1_epi32(0x00003F00);

	// Pack bytes together:
	__m256i out = _mm256_srli_epi32(in, 16);

	out = _mm256_or_si256(out, _mm256_srli_epi32(_mm256_and_si256(in, maskB2), 2));

	out = _mm256_or_si256(out, _mm256_slli_epi32(_mm256_and_si256(in, maskB1), 12));

	out = _mm256_or_si256(out, _mm256_slli_epi32(in, 26));

	// Pack bytes together within 32-bit words, discarding words 3 and 7:
	out = _mm256_shuffle_epi8(out, _mm256_setr_epi8(
		 3,  2,  1,
		 7,  6,  5,
		11, 10,  9,
		15, 14, 13,
		-1, -1, -1, -1,
		 3,  2,  1,
		 7,  6,  5,
		11, 10,  9,
		15, 14, 13,
		-1, -1, -1, -1));

	// Pack 32-bit words together, squashing empty words 3 and 7:
	return _mm256_permutevar8x32_epi32(out, _mm256_setr_epi32(
		0, 1, 2, 4, 5, 6, -1, -1));
}

#endif	// __AVX2__

BASE64_ENC_FUNCTION(avx2)
{
#ifdef __AVX2__
	#include "../generic/enc_head.c"
	#include "enc_loop.c"
	#include "../generic/enc_tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(avx2)
{
#ifdef __AVX2__
	#include "../generic/dec_head.c"
	#include "dec_loop.c"
	#include "../generic/dec_tail.c"
#else
	BASE64_DEC_STUB
#endif
}
