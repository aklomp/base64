/* If we have AVX2 support, pick off 24 bytes at a time for as long as we can.
 * But because we read 32 bytes at a time, ensure we have enough room to do a
 * full 32-byte read without segfaulting: */
while (srclen >= 32)
{
	__m128i l0, l1;
	__m256i str, mask, res, blockmask;
	__m256i s2mask, s3mask, s4mask, s5mask;

	/* _mm256_shuffle_epi8 works on 128-bit lanes, so we need to get
	 * the two 128-bit lanes into big-endian order separately: */
	l0 = _mm_loadu_si128((__m128i *)c);
	l0 = _mm_shuffle_epi8(l0,
	     _mm_setr_epi8(2, 2, 1, 0, 5, 5, 4, 3, 8, 8, 7, 6, 11, 11, 10, 9));

	l1 = _mm_loadu_si128((__m128i *)&c[12]);
	l1 = _mm_shuffle_epi8(l1,
	     _mm_setr_epi8(2, 2, 1, 0, 5, 5, 4, 3, 8, 8, 7, 6, 11, 11, 10, 9));

	/* Combine into a single 256-bit register: */
	str = _mm256_castsi128_si256(l0);
	str = _mm256_insertf128_si256(str, l1, 1);

	/* Mask to pass through only the lower 6 bits of one byte: */
	mask = _mm256_set1_epi32(0x3F000000);

	/* Shift bits by 2, mask in only the first byte: */
	res = _mm256_srli_epi32(str, 2) & mask;
	mask = _mm256_srli_epi32(mask, 8);

	/* Shift bits by 4, mask in only the second byte: */
	res |= _mm256_srli_epi32(str, 4) & mask;
	mask = _mm256_srli_epi32(mask, 8);

	/* Shift bits by 6, mask in only the third byte: */
	res |= _mm256_srli_epi32(str, 6) & mask;
	mask = _mm256_srli_epi32(mask, 8);

	/* No shift necessary for the fourth byte because we duplicated
	 * the third byte to this position; just mask: */
	res |= str & mask;

	/* Reorder to 32-bit little-endian: */
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

	/* The bits have now been shifted to the right locations;
	 * translate their values 0..63 to the Base64 alphabet.
	 * Because AVX2 can only compare 'greater than', start from end of alphabet: */

	/* set 5: 63, "/" */
	s5mask = _mm256_cmpeq_epi8(res, _mm256_set1_epi8(63));
	blockmask = s5mask;

	/* set 4: 62, "+" */
	s4mask = _mm256_cmpeq_epi8(res, _mm256_set1_epi8(62));
	blockmask |= s4mask;

	/* set 3: 52..61, "0123456789" */
	s3mask = _mm256_andnot_si256(blockmask, _mm256_cmpgt_epi8(res, _mm256_set1_epi8(51)));
	blockmask |= s3mask;

	/* set 2: 26..51, "abcdefghijklmnopqrstuvwxyz" */
	s2mask = _mm256_andnot_si256(blockmask, _mm256_cmpgt_epi8(res, _mm256_set1_epi8(25)));
	blockmask |= s2mask;

	/* set 1: 0..25, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	 * Everything that is not blockmasked */

	/* Create the masked character sets: */
	str = _mm256_set1_epi8('/') & s5mask;
	str = _mm256_blendv_epi8(str, _mm256_set1_epi8('+'), s4mask);
	str = _mm256_blendv_epi8(str, _mm256_add_epi8(res, _mm256_set1_epi8('0' - 52)), s3mask);
	str = _mm256_blendv_epi8(str, _mm256_add_epi8(res, _mm256_set1_epi8('a' - 26)), s2mask);
	str = _mm256_blendv_epi8(_mm256_add_epi8(res, _mm256_set1_epi8('A')), str, blockmask);

	/* Blend all the sets together and store: */
	_mm256_storeu_si256((__m256i *)o, str);

	c += 24;	/* 6 * 4 bytes of input  */
	o += 32;	/* 8 * 4 bytes of output */
	outl += 32;
	srclen -= 24;
}
