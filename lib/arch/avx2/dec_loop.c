static inline void
dec_loop_avx2 (const uint8_t **s, size_t *slen, uint8_t **o, size_t *olen)
{
	if (*slen < 45) {
		return;
	}

	// Process blocks of 32 bytes per round. Because 8 extra zero bytes are
	// written after the output, ensure that there will be at least 13
	// bytes of input data left to cover the gap. (11 data bytes and up to
	// two end-of-string markers.)
	size_t rounds = (*slen - 13) / 32;

	*slen -= rounds * 32;	// 32 bytes consumed per round
	*olen += rounds * 24;	// 24 bytes produced per round

	const __m256i lut_lo = _mm256_setr_epi8(
		0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
		0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A,
		0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
		0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A);

	const __m256i lut_hi = _mm256_setr_epi8(
		0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10);

	const __m256i lut_roll = _mm256_setr_epi8(
		0,  16,  19,   4, -65, -65, -71, -71,
		0,   0,   0,   0,   0,   0,   0,   0,
		0,  16,  19,   4, -65, -65, -71, -71,
		0,   0,   0,   0,   0,   0,   0,   0);

	const __m256i mask_2F = _mm256_set1_epi8(0x2F);

	do {
		// Load input:
		__m256i str = _mm256_loadu_si256((__m256i *) *s);

		// See the SSSE3 decoder for an explanation of the algorithm.
		const __m256i hi_nibbles = _mm256_and_si256(_mm256_srli_epi32(str, 4), mask_2F);
		const __m256i lo_nibbles = _mm256_and_si256(str, mask_2F);
		const __m256i hi         = _mm256_shuffle_epi8(lut_hi, hi_nibbles);
		const __m256i lo         = _mm256_shuffle_epi8(lut_lo, lo_nibbles);

		if (!_mm256_testz_si256(lo, hi)) {
			break;
		}

		const __m256i eq_2F = _mm256_cmpeq_epi8(str, mask_2F);
		const __m256i roll  = _mm256_shuffle_epi8(lut_roll, _mm256_add_epi8(eq_2F, hi_nibbles));

		// Now simply add the delta values to the input:
		str = _mm256_add_epi8(str, roll);

		// Reshuffle the input to packed 12-byte output format:
		str = dec_reshuffle(str);

		// Store the output:
		_mm256_storeu_si256((__m256i *) *o, str);

		*s += 32;
		*o += 24;

	} while (--rounds > 0);

	// Adjust for any rounds that were skipped:
	*slen += rounds * 32;
	*olen -= rounds * 24;
}
