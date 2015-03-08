/* If we have NEON support on 32-bit ARM, pick off 32 bytes at a time for as
 * long as we can. We use 64-bit wide vectors instead of 128-bit wide ones,
 * because this routine consumes a lot of registers. By working in 64-bit
 * space, we have enough registers to avoid using the stack.
 * Unlike the SSE codecs, we don't write trailing zero bytes to output, so we
 * don't need to check if we have enough remaining input to cover them: */
while (srclen >= 32)
{
	uint8x8_t classified;
	uint8x8x4_t s1mask, s2mask, s3mask, s4mask, s5mask;
	uint8x8x4_t str, res;
	uint8x8x3_t dec;

	/* Load 32 bytes and deinterleave: */
	str = vld4_u8((uint8_t *)c);

	/* Classify characters into five sets:
	 * Set 1: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	s1mask.val[0] = vcge_u8(str.val[0], vdup_n_u8('A')) & vcle_u8(str.val[0], vdup_n_u8('Z'));
	s1mask.val[1] = vcge_u8(str.val[1], vdup_n_u8('A')) & vcle_u8(str.val[1], vdup_n_u8('Z'));
	s1mask.val[2] = vcge_u8(str.val[2], vdup_n_u8('A')) & vcle_u8(str.val[2], vdup_n_u8('Z'));
	s1mask.val[3] = vcge_u8(str.val[3], vdup_n_u8('A')) & vcle_u8(str.val[3], vdup_n_u8('Z'));

	/* Set 2: "abcdefghijklmnopqrstuvwxyz" */
	s2mask.val[0] = vcge_u8(str.val[0], vdup_n_u8('a')) & vcle_u8(str.val[0], vdup_n_u8('z'));
	s2mask.val[1] = vcge_u8(str.val[1], vdup_n_u8('a')) & vcle_u8(str.val[1], vdup_n_u8('z'));
	s2mask.val[2] = vcge_u8(str.val[2], vdup_n_u8('a')) & vcle_u8(str.val[2], vdup_n_u8('z'));
	s2mask.val[3] = vcge_u8(str.val[3], vdup_n_u8('a')) & vcle_u8(str.val[3], vdup_n_u8('z'));

	/* Set 3: "0123456789" */
	s3mask.val[0] = vcge_u8(str.val[0], vdup_n_u8('0')) & vcle_u8(str.val[0], vdup_n_u8('9'));
	s3mask.val[1] = vcge_u8(str.val[1], vdup_n_u8('0')) & vcle_u8(str.val[1], vdup_n_u8('9'));
	s3mask.val[2] = vcge_u8(str.val[2], vdup_n_u8('0')) & vcle_u8(str.val[2], vdup_n_u8('9'));
	s3mask.val[3] = vcge_u8(str.val[3], vdup_n_u8('0')) & vcle_u8(str.val[3], vdup_n_u8('9'));

	/* Set 4: "+" */
	s4mask.val[0] = vceq_u8(str.val[0], vdup_n_u8('+'));
	s4mask.val[1] = vceq_u8(str.val[1], vdup_n_u8('+'));
	s4mask.val[2] = vceq_u8(str.val[2], vdup_n_u8('+'));
	s4mask.val[3] = vceq_u8(str.val[3], vdup_n_u8('+'));

	/* Set 5: "/" */
	s5mask.val[0] = vceq_u8(str.val[0], vdup_n_u8('/'));
	s5mask.val[1] = vceq_u8(str.val[1], vdup_n_u8('/'));
	s5mask.val[2] = vceq_u8(str.val[2], vdup_n_u8('/'));
	s5mask.val[3] = vceq_u8(str.val[3], vdup_n_u8('/'));

	/* Check if all bytes have been classified;
	 * else fall back on bytewise code to do error checking and reporting: */
	classified  = (s1mask.val[0] | s2mask.val[0] | s3mask.val[0] | s4mask.val[0] | s5mask.val[0]);
	classified &= (s1mask.val[1] | s2mask.val[1] | s3mask.val[1] | s4mask.val[1] | s5mask.val[1]);
	classified &= (s1mask.val[2] | s2mask.val[2] | s3mask.val[2] | s4mask.val[2] | s5mask.val[2]);
	classified &= (s1mask.val[3] | s2mask.val[3] | s3mask.val[3] | s4mask.val[3] | s5mask.val[3]);
	classified  = vmvn_u8(classified);

	/* Extract both 32-bit halves; check that all bits are zero: */
	if (vget_lane_u32((uint32x2_t)classified, 0) != 0
	 || vget_lane_u32((uint32x2_t)classified, 1) != 0) {
		break;
	}
	/* Subtract sets from byte values: */
	res.val[0] = s1mask.val[0] & vsub_u8(str.val[0], vdup_n_u8('A'));
	res.val[1] = s1mask.val[1] & vsub_u8(str.val[1], vdup_n_u8('A'));
	res.val[2] = s1mask.val[2] & vsub_u8(str.val[2], vdup_n_u8('A'));
	res.val[3] = s1mask.val[3] & vsub_u8(str.val[3], vdup_n_u8('A'));

	res.val[0] = vbsl_u8(s2mask.val[0], vsub_u8(str.val[0], vdup_n_u8('a' - 26)), res.val[0]);
	res.val[1] = vbsl_u8(s2mask.val[1], vsub_u8(str.val[1], vdup_n_u8('a' - 26)), res.val[1]);
	res.val[2] = vbsl_u8(s2mask.val[2], vsub_u8(str.val[2], vdup_n_u8('a' - 26)), res.val[2]);
	res.val[3] = vbsl_u8(s2mask.val[3], vsub_u8(str.val[3], vdup_n_u8('a' - 26)), res.val[3]);

	res.val[0] = vbsl_u8(s3mask.val[0], vsub_u8(str.val[0], vdup_n_u8('0' - 52)), res.val[0]);
	res.val[1] = vbsl_u8(s3mask.val[1], vsub_u8(str.val[1], vdup_n_u8('0' - 52)), res.val[1]);
	res.val[2] = vbsl_u8(s3mask.val[2], vsub_u8(str.val[2], vdup_n_u8('0' - 52)), res.val[2]);
	res.val[3] = vbsl_u8(s3mask.val[3], vsub_u8(str.val[3], vdup_n_u8('0' - 52)), res.val[3]);

	res.val[0] = vbsl_u8(s4mask.val[0], vdup_n_u8(62), res.val[0]);
	res.val[1] = vbsl_u8(s4mask.val[1], vdup_n_u8(62), res.val[1]);
	res.val[2] = vbsl_u8(s4mask.val[2], vdup_n_u8(62), res.val[2]);
	res.val[3] = vbsl_u8(s4mask.val[3], vdup_n_u8(62), res.val[3]);

	res.val[0] = vbsl_u8(s5mask.val[0], vdup_n_u8(63), res.val[0]);
	res.val[1] = vbsl_u8(s5mask.val[1], vdup_n_u8(63), res.val[1]);
	res.val[2] = vbsl_u8(s5mask.val[2], vdup_n_u8(63), res.val[2]);
	res.val[3] = vbsl_u8(s5mask.val[3], vdup_n_u8(63), res.val[3]);

	/* Compress four bytes into three: */
	dec.val[0] = vshl_n_u8(res.val[0], 2) | vshr_n_u8(res.val[1], 4);
	dec.val[1] = vshl_n_u8(res.val[1], 4) | vshr_n_u8(res.val[2], 2);
	dec.val[2] = vshl_n_u8(res.val[2], 6) | res.val[3];

	/* Interleave and store decoded result: */
	vst3_u8((uint8_t *)o, dec);

	c += 32;
	o += 24;
	outl += 24;
	srclen -= 32;
}
