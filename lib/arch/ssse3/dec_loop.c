// If we have SSSE3 support, pick off 16 bytes at a time for as long as we can,
// but make sure that we quit before seeing any == markers at the end of the
// string. Also, because we write four zeroes at the end of the output, ensure
// that there are at least 6 valid bytes of input data remaining to close the
// gap. 16 + 2 + 6 = 24 bytes:
while (srclen >= 24)
{
	const __m128i delta_asso = _mm_setr_epi8(
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F
	);
	const __m128i delta_values = _mm_setr_epi8(
			0x00, 0x00, 0x00, 0x13, 0x04, 0xBF, 0xBF, 0xB9,
			0xB9, 0x00, 0x10, 0xC3, 0xBF, 0xBF, 0xB9, 0xB9
	);
	const __m128i check_asso = _mm_setr_epi8(
			0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x01, 0x01, 0x03, 0x07, 0x0B, 0x0B, 0x0B, 0x0F
	);
	const __m128i check_values = _mm_setr_epi8(
			0x80, 0x80, 0x80, 0x80, 0xCF, 0xBF, 0xD5, 0xA6,
			0xB5, 0x86, 0xD1, 0x80, 0xB1, 0x80, 0x91, 0x80
	);

	// Load string:
	__m128i asrc = _mm_loadu_si128((__m128i *)c);

	// Map input bytes to 6-bit values:
	const __m128i shifted = _mm_srli_epi32(asrc, 3);
	const __m128i delta_hash = _mm_avg_epu8(_mm_shuffle_epi8(delta_asso, asrc), shifted);
	__m128i out = _mm_add_epi8(_mm_shuffle_epi8(delta_values, delta_hash), asrc);

	// Detect invalid input bytes
	const __m128i check_hash = _mm_avg_epu8(_mm_shuffle_epi8(check_asso, asrc), shifted);
	const __m128i chk = _mm_adds_epi8(_mm_shuffle_epi8(check_values, check_hash), asrc);

	// Fall back on bytewise code to do error checking and reporting:
	if (_mm_movemask_epi8(chk) != 0) {
		break;
	}

	// Pack 16 bytes into the 12-byte output format and store back:
	_mm_storeu_si128((__m128i *)o, dec_reshuffle(out));

	c += 16;
	o += 12;
	outl += 12;
	srclen -= 16;
}
