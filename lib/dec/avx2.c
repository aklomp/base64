/* If we have AVX2 support, pick off 32 bytes at a time for as long
 * as we can, but make sure that we quit before seeing any == markers
 * at the end of the string. Also, because we write 8 zeroes at
 * the end of the output, ensure that there are at least 11 valid bytes
 * of input data remaining to close the gap. 32 + 2 + 11 = 45 bytes: */
while (srclen >= 45)
{
	__m128i l0, l1;
	__m256i str, mask, res;
	__m256i s1mask, s2mask, s3mask, s4mask, s5mask;

	/* Load string: */
	str = _mm256_loadu_si256((__m256i *)c);

	/* Classify characters into five sets:
	 * Set 1: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	s1mask = _mm256_andnot_si256(
			_mm256_cmpgt_epi8(str, _mm256_set1_epi8('Z')),
			_mm256_cmpgt_epi8(str, _mm256_set1_epi8('A' - 1)));

	/* Set 2: "abcdefghijklmnopqrstuvwxyz" */
	s2mask = _mm256_andnot_si256(
			_mm256_cmpgt_epi8(str, _mm256_set1_epi8('z')),
			_mm256_cmpgt_epi8(str, _mm256_set1_epi8('a' - 1)));

	/* Set 3: "0123456789" */
	s3mask = _mm256_andnot_si256(
			_mm256_cmpgt_epi8(str, _mm256_set1_epi8('9')),
			_mm256_cmpgt_epi8(str, _mm256_set1_epi8('0' - 1)));

	/* Set 4: "+" */
	s4mask = _mm256_cmpeq_epi8(str, _mm256_set1_epi8('+'));

	/* Set 5: "/" */
	s5mask = _mm256_cmpeq_epi8(str, _mm256_set1_epi8('/'));

	/* Check if all bytes have been classified; else fall back on bytewise code
	 * to do error checking and reporting: */
	if ((unsigned int)_mm256_movemask_epi8(s1mask | s2mask | s3mask | s4mask | s5mask) != 0xFFFFFFFF) {
		break;
	}
	/* Subtract sets from byte values: */
	res = _mm256_sub_epi8(str, _mm256_set1_epi8('A')) & s1mask;
	res = _mm256_blendv_epi8(res, _mm256_sub_epi8(str, _mm256_set1_epi8('a' - 26)), s2mask);
	res = _mm256_blendv_epi8(res, _mm256_sub_epi8(str, _mm256_set1_epi8('0' - 52)), s3mask);
	res = _mm256_blendv_epi8(res, _mm256_set1_epi8(62), s4mask);
	res = _mm256_blendv_epi8(res, _mm256_set1_epi8(63), s5mask);

	/* Shuffle bytes to 32-bit bigendian: */
	res = _mm256_shuffle_epi8(res,
	      _mm256_setr_epi8(
			 3,  2,  1,  0,
			 7,  6,  5,  4,
			11, 10,  9,  8,
			15, 14, 13, 12,
			 3,  2,  1,  0,
			 7,  6,  5,  4,
			11, 10,  9,  8,
			15, 14, 13, 12));

	/* Mask in a single byte per shift: */
	mask = _mm256_set1_epi32(0x3F000000);

	/* Pack bytes together: */
	str = _mm256_slli_epi32(res & mask, 2);
	mask = _mm256_srli_epi32(mask, 8);

	str |= _mm256_slli_epi32(res & mask, 4);
	mask = _mm256_srli_epi32(mask, 8);

	str |= _mm256_slli_epi32(res & mask, 6);
	mask = _mm256_srli_epi32(mask, 8);

	str |= _mm256_slli_epi32(res & mask, 8);

	/* As in AVX2 encoding, we have to shuffle and repack each 128-bit lane
	 * separately due to the way _mm256_shuffle_epi8 works: */
	l0 = _mm_shuffle_epi8(
	     _mm256_extractf128_si256(str, 0),
	     _mm_setr_epi8(
			 3,  2,  1,
			 7,  6,  5,
			11, 10,  9,
			15, 14, 13,
			-1, -1, -1, -1));

	l1 = _mm_shuffle_epi8(
	     _mm256_extractf128_si256(str, 1),
	     _mm_setr_epi8(
			 3,  2,  1,
			 7,  6,  5,
			11, 10,  9,
			15, 14, 13,
			-1, -1, -1, -1));

	/* Store back: */
	_mm_storeu_si128((__m128i *)o, l0);
	_mm_storeu_si128((__m128i *)&o[12], l1);

	c += 32;
	o += 24;
	outl += 24;
	srclen -= 32;
}
