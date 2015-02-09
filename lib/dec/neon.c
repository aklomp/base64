/* If we have NEON support, pick off 16 bytes at a time for as long
 * as we can, but make sure that we quit before seeing any == markers
 * at the end of the string. Also, because we write four zeroes at
 * the end of the output, ensure that there are at least 6 valid bytes
 * of input data remaining to close the gap. 16 + 2 + 6 = 24 bytes: */
while (srclen >= 24)
{
	uint8x16_t str, res;
	uint32x4_t mask;
	uint8x16_t s1mask, s2mask, s3mask, s4mask, s5mask;
	uint32x2_t classified;
	uint8_t t[8] = { 3, 2, 1, 7, 6, 5, 255, 255 };
	uint8x8_t table, lo, hi;

	/* Load string: */
	str = vld1q_u8((void *)c);

	/* Classify characters into five sets:
	 * Set 1: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	s1mask = vandq_u8(vcgeq_u8(str, vdupq_n_u8('A')),  /* >= A */
			  vcleq_u8(str, vdupq_n_u8('Z'))); /* <= Z */

	/* Set 2: "abcdefghijklmnopqrstuvwxyz" */
	s2mask = vandq_u8(vcgeq_u8(str, vdupq_n_u8('a')),  /* >= a */
			  vcleq_u8(str, vdupq_n_u8('z'))); /* <= z */

	/* Set 3: "0123456789" */
	s3mask = vandq_u8(vcgeq_u8(str, vdupq_n_u8('0')),  /* >= 0 */
			  vcleq_u8(str, vdupq_n_u8('9'))); /* <= 9 */

	/* Set 4: "+" */
	s4mask = vceqq_u8(str, vdupq_n_u8('+'));

	/* Set 5: "/" */
	s5mask = vceqq_u8(str, vdupq_n_u8('/'));

	/* Check if all bytes have been classified; else fall back on bytewise code
	 * to do error checking and reporting.
	 * Combine all masks, then shift the result right by four and narrow.
	 * This leaves a 64-bit vector with 4 flag bits per character: */
	classified = (uint32x2_t)vshrn_n_u16((uint16x8_t)(s1mask | s2mask | s3mask | s4mask | s5mask), 4);

	/* Extract both 32-bit halves; check that all bits are set: */
	if (vget_lane_u32(classified, 0) != 0xFFFFFFFF) {
		break;
	}
	if (vget_lane_u32(classified, 1) != 0xFFFFFFFF) {
		break;
	}
	/* Subtract sets from byte values: */
	res  = s1mask & vsubq_u8(str, vdupq_n_u8('A'));
	res |= s2mask & vsubq_u8(str, vdupq_n_u8('a' - 26));
	res |= s3mask & vsubq_u8(str, vdupq_n_u8('0' - 52));
	res |= s4mask & vdupq_n_u8(62);
	res |= s5mask & vdupq_n_u8(63);

	/* Shuffle bytes to 32-bit bigendian: */
	res = vrev32q_u8(res);

	/* Mask in a single byte per shift: */
	mask = vdupq_n_u32(0x3F000000);

	/* Pack bytes together: */
	str  = (uint8x16_t)vshlq_n_u32((uint32x4_t)res & mask, 2);
	mask = vshrq_n_u32(mask, 8);

	str |= (uint8x16_t)vshlq_n_u32((uint32x4_t)res & mask, 4);
	mask = vshrq_n_u32(mask, 8);

	str |= (uint8x16_t)vshlq_n_u32((uint32x4_t)res & mask, 6);
	mask = vshrq_n_u32(mask, 8);

	str |= (uint8x16_t)vshlq_n_u32((uint32x4_t)res & mask, 8);

	/* Reshuffle and repack into 12-byte output format.
	 * If we read in the string:
	 *
	 *   ABCDEFGHIJKLMNOP
	 *
	 * it will be represented in memory as:
	 *
	 *   PONMLKJIHGFEDCBA
	 *      |   |   |   |
	 *     12   8   4   0
	 *
	 * We have already done a 32-bit wide bswap on the string:
	 *
	 *   MNOPIJKLEFGHABCD
	 *
	 * And by shifting and merging, eliminated every fourth byte:
	 *
	 *   MNO.IJK.EFG.ABC.
	 *
	 * We now want to reverse and "deflate" the string to:
	 *
	 *   ....ONMKJIGFECBA
	 *
	 * because if we write this out, we are done. This needs
	 * a byte reordering. The code below reorders the high
	 * and low halves with first a table lookup:
	 *
	 *   MNO.IJK. -> ..ONMKJI  (hi)
	 *   EFG.ABC. -> ..GFECBA  (lo)
	 *
	 * Then it shifts the high byte left by six bytes and
	 * or's it with the lower byte:
	 *
	 *   JIGFECBA (lo)
	 *
	 * Then it shifts the high byte right two bytes:
	 *
	 *   ....ONMK (hi)
	 *
	 * and finally combines the two halves into the desired vector.
	 *
	 */
	table = vld1_u8((uint8_t *)t);
	lo = vtbl1_u8(vget_low_u8(str), table);
	hi = vtbl1_u8(vget_high_u8(str), table);

	lo = vorr_u8(lo, (uint8x8_t)vshl_n_u64((uint64x1_t)hi, 48));
	str = vcombine_u8(lo, (uint8x8_t)vshr_n_u64((uint64x1_t)hi, 16));

	/* Store back: */
	vst1q_u8((void *)o, str);

	c += 16;
	o += 12;
	outl += 12;
	srclen -= 16;
}
