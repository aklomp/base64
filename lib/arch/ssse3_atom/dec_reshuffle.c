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
