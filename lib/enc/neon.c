/* If we have ARM NEON support, pick off 12 bytes at a time for as long as we
 * can. But because we read 16 bytes at a time, ensure we have enough room to
 * do a full 16-byte read without segfaulting: */
while (srclen >= 16)
{
	uint8x16_t res;
	uint32x4_t str, mask;
	uint8x16_t s1, s2, s3, s4, s5;
	uint8x16_t s1mask, s2mask, s3mask, s4mask;
	uint8x16_t blockmask;
	uint8x8_t tbl_shuf, lo, hi;
	uint8_t t[8] = { 2, 2, 1, 0, 5, 5, 4, 3 };

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
	 * translate their values 0..63 to the Base64 alphabet: */

	/* set 1: 0..25, "ABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	s1mask = vcltq_u8(res, vdupq_n_u8(26));
	blockmask = s1mask;

	/* set 2: 26..51, "abcdefghijklmnopqrstuvwxyz" */
	s2mask = vmvnq_u8(blockmask) & vcltq_u8(res, vdupq_n_u8(52));
	blockmask |= s2mask;

	/* set 3: 52..61, "0123456789" */
	s3mask = vmvnq_u8(blockmask) & vcltq_u8(res, vdupq_n_u8(62));
	blockmask |= s3mask;

	/* set 4: 62, "+" */
	s4mask = vceqq_u8(res, vdupq_n_u8(62));
	blockmask |= s4mask;

	/* set 5: 63, "/"
	 * Everything that is not blockmasked */

	/* Create the masked character sets: */
	s1 = s1mask & vaddq_u8(res, vdupq_n_u8('A'));
	s2 = s2mask & vaddq_u8(res, vdupq_n_u8('a' - 26));
	s3 = s3mask & vaddq_u8(res, vdupq_n_u8('0' - 52));
	s4 = s4mask & vdupq_n_u8('+');
	s5 = vmvnq_u8(blockmask) & vdupq_n_u8('/');

	/* Blend all the sets together and store: */
	vst1q_u8((uint8_t *)o, s1 | s2 | s3 | s4 | s5);

	c += 12;	/* 3 * 4 bytes of input  */
	o += 16;	/* 4 * 4 bytes of output */
	outl += 16;
	srclen -= 12;
}
