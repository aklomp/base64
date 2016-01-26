// If we have AVX2 support, pick off 24 bytes at a time for as long as we can.
// But because we read 32 bytes at a time, ensure we have enough room to do a
// full 32-byte read without segfaulting:
while (srclen >= 32)
{
	// Load string as two segments of 12 bytes:
	__m128i lo = _mm_loadu_si128((__m128i *) &c[ 0]);
	__m128i hi = _mm_loadu_si128((__m128i *) &c[12]);

	// Reshuffle:
	__m256i str = enc_reshuffle(lo, hi);

	// Translate reshuffled bytes to the Base64 alphabet:
	str = enc_translate(str);

	// Store:
	_mm256_storeu_si256((__m256i *)o, str);

	c += 24;	// 6 * 4 bytes of input
	o += 32;	// 8 * 4 bytes of output
	outl += 32;
	srclen -= 24;
}
