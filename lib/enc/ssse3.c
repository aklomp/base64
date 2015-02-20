/* If we have SSSE3 support, pick off 12 bytes at a time for as long as we can.
 * But because we read 16 bytes at a time, ensure we have enough room to do a
 * full 16-byte read without segfaulting: */
while (srclen >= 16)
{
	__m128i str, mask, res, blockmask;
	__m128i s1, s2, s3, s4, s5;
	__m128i s1mask, s2mask, s3mask, s4mask;

	/* Load string: */
	str = _mm_loadu_si128((__m128i *)c);

	/* Reorder to 32-bit big-endian, duplicating the third byte in every block of four.
	 * This copies the third byte to its final destination, so we can include it later
	 * by just masking instead of shifting and masking.
	 * The workset must be in big-endian, otherwise the shifted bits do not carry over
	 * properly among adjacent bytes: */
	str = _mm_shuffle_epi8(str,
	      _mm_setr_epi8(2, 2, 1, 0, 5, 5, 4, 3, 8, 8, 7, 6, 11, 11, 10, 9));

	/* Mask to pass through only the lower 6 bits of one byte: */
	mask = _mm_set1_epi32(0x3F000000);

	/* Shift bits by 2, mask in only the first byte: */
	res = _mm_srli_epi32(str, 2) & mask;
	mask = _mm_srli_epi32(mask, 8);

	/* Shift bits by 4, mask in only the second byte: */
	res |= _mm_srli_epi32(str, 4) & mask;
	mask = _mm_srli_epi32(mask, 8);

	/* Shift bits by 6, mask in only the third byte: */
	res |= _mm_srli_epi32(str, 6) & mask;
	mask = _mm_srli_epi32(mask, 8);

	/* No shift necessary for the fourth byte because we duplicated
	 * the third byte to this position; just mask: */
	res |= str & mask;

	/* Reorder to 32-bit little-endian: */
	res = _mm_shuffle_epi8(res,
	      _mm_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12));

	/* The bits have now been shifted to the right locations;
	 * translate their values 0..63 to the Base64 alphabet: */

	/* set 1: 0..25, "ABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	s1mask = _mm_cmplt_epi8(res, _mm_set1_epi8(26));
	blockmask = s1mask;

	/* set 2: 26..51, "abcdefghijklmnopqrstuvwxyz" */
	s2mask = _mm_andnot_si128(blockmask, _mm_cmplt_epi8(res, _mm_set1_epi8(52)));
	blockmask |= s2mask;

	/* set 3: 52..61, "0123456789" */
	s3mask = _mm_andnot_si128(blockmask, _mm_cmplt_epi8(res, _mm_set1_epi8(62)));
	blockmask |= s3mask;

	/* set 4: 62, "+" */
	s4mask = _mm_andnot_si128(blockmask, _mm_cmplt_epi8(res, _mm_set1_epi8(63)));
	blockmask |= s4mask;

	/* set 5: 63, "/"
	 * Everything that is not blockmasked */

	/* Create the masked character sets: */
	s1 = s1mask & _mm_add_epi8(res, _mm_set1_epi8('A'));
	s2 = s2mask & _mm_add_epi8(res, _mm_set1_epi8('a' - 26));
	s3 = s3mask & _mm_add_epi8(res, _mm_set1_epi8('0' - 52));
	s4 = s4mask & _mm_set1_epi8('+');
	s5 = _mm_andnot_si128(blockmask, _mm_set1_epi8('/'));

	/* Blend all the sets together and store: */
	_mm_storeu_si128((__m128i *)o, s1 | s2 | s3 | s4 | s5);

	c += 12;	/* 3 * 4 bytes of input  */
	o += 16;	/* 4 * 4 bytes of output */
	outl += 16;
	srclen -= 12;
}
