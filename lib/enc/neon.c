/* If we have ARM NEON support, pick off 12 bytes at a time for as long as we
 * can. But because we read 16 bytes at a time, ensure we have enough room to
 * do a full 16-byte read without segfaulting: */
while (srclen >= 16)
{
	uint8x16_t res;
	uint32x4_t str, mask;
	uint8x8_t tbl_shuf, lo, hi;
	uint8_t t[8] = { 2, 2, 1, 0, 5, 5, 4, 3 };
	uint8x16_t loset, hiset, himask;

	/* Load string: */
	str = vld1q_u32((uint32_t *)c);

	/* Reorder to 32-bit big-endian.
	 * The workset must be in big-endian, otherwise the shifted bits do not
	 * carry over properly among adjacent bytes. If we read in the string:
	 *
	 *   ABCDEFGHIJKLMNOP
	 *
	 * it will be represented in memory as:
	 *
	 *   PONMLKJIHGFEDCBA
	 *      |   |   |   |
	 *     12   8   4   0
	 *
	 * For efficient processing, we want to reorder the bytes into:
	 *
	 *   JKLLGHIIDEFFABCC
	 *
	 * First we reorder the low half using a table:
	 *
	 *   HGFEDCBA -> DEFFABCC (lo)
	 *
	 * Then we left-rotate the whole string by 2 bytes:
	 *
	 *   NMLKJIHGFEDCBAPO
	 *
	 * Take the high half and reorder using the table:
	 *
	 *   NMLKJIHG -> JKLLGHII (hi)
	 *
	 * Then combine upper and lower into the desired vector.
	 *
	 */
	tbl_shuf = vld1_u8((uint8_t *)t);
	lo = vtbl1_u8(vget_low_u8((uint8x16_t)str), tbl_shuf);
	hi = vtbl1_u8(vget_high_u8(vextq_u8((uint8x16_t)str, (uint8x16_t)str, 14)), tbl_shuf);
	str = (uint32x4_t)vcombine_u8(lo, hi);

	/* Mask to pass through only the lower 6 bits of one byte: */
	mask = vdupq_n_u32(0x3F000000);

	/* Shift bits by 2, mask in only the first byte: */
	res = (uint8x16_t)(vshrq_n_u32(str, 2) & mask);
	mask = vshrq_n_u32(mask, 8);

	/* Shift bits by 4, mask in only the second byte: */
	res |= (uint8x16_t)(vshrq_n_u32(str, 4) & mask);
	mask = vshrq_n_u32(mask, 8);

	/* Shift bits by 6, mask in only the third byte: */
	res |= (uint8x16_t)(vshrq_n_u32(str, 6) & mask);
	mask = vshrq_n_u32(mask, 8);

	/* Shift bits by 8, mask in only the fourth byte: */
	res |= (uint8x16_t)(str & mask);

	/* Reorder to 32-bit little-endian: */
	res = vrev32q_u8(res);

	/* The bits have now been shifted to the right locations;
	 * translate their values 0..63 to the Base64 alphabet. */

	/* Make a mask for all bytes >= 32: */
	himask = vcgeq_u8(res, vdupq_n_u8(32));

	/* Make two sets; saturate-subtract 32 from high set: */
	loset = res;
	hiset = vqsubq_u8(res, vdupq_n_u8(32));

	/* Split sets into halves, do 32-bit table lookup on each half: */
	loset = vcombine_u8(
		vtbl4_u8(tbl_enc_lo, vget_low_u8(loset)),
		vtbl4_u8(tbl_enc_lo, vget_high_u8(loset))
	);
	hiset = vcombine_u8(
		vtbl4_u8(tbl_enc_hi, vget_low_u8(hiset)),
		vtbl4_u8(tbl_enc_hi, vget_high_u8(hiset))
	);
	/* Mask out unwanted results: */
	hiset &= himask;

	/* Combine and store result: */
	vst1q_u8((uint8_t *)o, hiset | loset);

	c += 12;	/* 3 * 4 bytes of input  */
	o += 16;	/* 4 * 4 bytes of output */
	outl += 16;
	srclen -= 12;
}
