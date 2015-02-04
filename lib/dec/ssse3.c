/* If we have SSSE3 support, pick off 16 bytes at a time for as long
 * as we can, but make sure that we quit before seeing any == markers
 * at the end of the string. Also, because we write four zeroes at
 * the end of the output, ensure that there are at least 6 valid bytes
 * of input data remaining to close the gap. 16 + 2 + 6 = 24 bytes: */
while (srclen >= 24)
{
	__m128i str, mask, res;
	__m128i s1mask, s2mask, s3mask, s4mask, s5mask;

	/* Load string: */
	str = _mm_loadu_si128((__m128i *)c);

	/* Classify characters into five sets:
	 * Set 1: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	s1mask = _mm_andnot_si128(
			_mm_cmplt_epi8(str, _mm_set1_epi8('A')),
			_mm_cmplt_epi8(str, _mm_set1_epi8('Z' + 1)));

	/* Set 2: "abcdefghijklmnopqrstuvwxyz" */
	s2mask = _mm_andnot_si128(
			_mm_cmplt_epi8(str, _mm_set1_epi8('a')),
			_mm_cmplt_epi8(str, _mm_set1_epi8('z' + 1)));

	/* Set 3: "0123456789" */
	s3mask = _mm_andnot_si128(
			_mm_cmplt_epi8(str, _mm_set1_epi8('0')),
			_mm_cmplt_epi8(str, _mm_set1_epi8('9' + 1)));

	/* Set 4: "+" */
	s4mask = _mm_cmpeq_epi8(str, _mm_set1_epi8('+'));

	/* Set 5: "/" */
	s5mask = _mm_cmpeq_epi8(str, _mm_set1_epi8('/'));

	/* Check if all bytes have been classified; else fall back on bytewise code
	 * to do error checking and reporting: */
	if (_mm_movemask_epi8(s1mask | s2mask | s3mask | s4mask | s5mask) != 0xFFFF) {
		break;
	}
	/* Subtract sets from byte values: */
	res  = s1mask & _mm_sub_epi8(str, _mm_set1_epi8('A'));
	res |= s2mask & _mm_sub_epi8(str, _mm_set1_epi8('a' - 26));
	res |= s3mask & _mm_sub_epi8(str, _mm_set1_epi8('0' - 52));
	res |= s4mask & _mm_set1_epi8(62);
	res |= s5mask & _mm_set1_epi8(63);

	/* Shuffle bytes to 32-bit bigendian: */
	res = _mm_shuffle_epi8(res,
	      _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12));

	/* Mask in a single byte per shift: */
	mask = _mm_set1_epi32(0x3F000000);

	/* Pack bytes together: */
	str = _mm_slli_epi32(res & mask, 2);
	mask = _mm_srli_epi32(mask, 8);

	str |= _mm_slli_epi32(res & mask, 4);
	mask = _mm_srli_epi32(mask, 8);

	str |= _mm_slli_epi32(res & mask, 6);
	mask = _mm_srli_epi32(mask, 8);

	str |= _mm_slli_epi32(res & mask, 8);

	/* Reshuffle and repack into 12-byte output format: */
	str = _mm_shuffle_epi8(str,
	      _mm_setr_epi8(3, 2, 1, 7, 6, 5, 11, 10, 9, 15, 14, 13, -1, -1, -1, -1));

	/* Store back: */
	_mm_storeu_si128((__m128i *)o, str);

	c += 16;
	o += 12;
	outl += 12;
	srclen -= 16;
}
