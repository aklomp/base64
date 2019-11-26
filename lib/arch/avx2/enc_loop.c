static inline void
enc_loop_avx2 (const uint8_t **s, size_t *slen, uint8_t **o, size_t *olen)
{
	if (*slen < 32) {
		return;
	}

	// Process blocks of 24 bytes at a time. Because blocks are loaded 32
	// bytes at a time an offset of -4, ensure that there will be at least
	// 4 remaining bytes after the last round, so that the final read will
	// not pass beyond the bounds of the input buffer:
	size_t rounds = (*slen - 4) / 24;

	*slen -= rounds * 24;   // 24 bytes consumed per round
	*olen += rounds * 32;   // 32 bytes produced per round

	// First load is done at s - 0 to not get a segfault:
	__m256i inputvector = _mm256_loadu_si256((__m256i *) *s);

	// Subsequent loads will be done at s - 4, set pointer for next round:
	*s += 20;

	// Shift by 4 bytes, as required by enc_reshuffle:
	inputvector = _mm256_permutevar8x32_epi32(inputvector, _mm256_setr_epi32(0, 0, 1, 2, 3, 4, 5, 6));

	for (;;) {

		// Reshuffle, translate, store:
		inputvector = enc_reshuffle(inputvector);
		inputvector = enc_translate(inputvector);
		_mm256_storeu_si256((__m256i *) *o, inputvector);
		*o += 32;

		if (--rounds == 0) {
			break;
		}

		// Load for the next round:
		inputvector = _mm256_loadu_si256((__m256i *) *s);
		*s += 24;
	}

	// Add the offset back:
	*s += 4;
}
