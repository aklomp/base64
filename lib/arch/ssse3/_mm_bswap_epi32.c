static inline __m128i
_mm_bswap_epi32 (const __m128i in)
{
	return _mm_shuffle_epi8(in, _mm_setr_epi8(
		 3,  2,  1,  0,
		 7,  6,  5,  4,
		11, 10,  9,  8,
		15, 14, 13, 12));
}
