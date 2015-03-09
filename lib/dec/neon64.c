/* If we have NEON 64-bit support, pick off 64 bytes at a time for as long as
 * we can. Unlike the SSE codecs, we don't write trailing zero bytes to output,
 * so we don't need to check if we have enough remaining input to cover them: */
while (srclen >= 64)
{
	uint8x16_t classified;
	uint32x2_t narrowed;
	uint8x16x4_t s1mask, s2mask, s3mask, s4mask, s5mask;
	uint8x16x4_t str, res;
	uint8x16x3_t dec;

	/* Load 64 bytes and deinterleave: */
	str = vld4q_u8((uint8_t *)c);

	/* Classify characters into five sets:
	 * Set 1: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" */
	s1mask.val[0] = vcgeq_u8(str.val[0], vdupq_n_u8('A')) & vcleq_u8(str.val[0], vdupq_n_u8('Z'));
	s1mask.val[1] = vcgeq_u8(str.val[1], vdupq_n_u8('A')) & vcleq_u8(str.val[1], vdupq_n_u8('Z'));
	s1mask.val[2] = vcgeq_u8(str.val[2], vdupq_n_u8('A')) & vcleq_u8(str.val[2], vdupq_n_u8('Z'));
	s1mask.val[3] = vcgeq_u8(str.val[3], vdupq_n_u8('A')) & vcleq_u8(str.val[3], vdupq_n_u8('Z'));

	/* Set 2: "abcdefghijklmnopqrstuvwxyz" */
	s2mask.val[0] = vcgeq_u8(str.val[0], vdupq_n_u8('a')) & vcleq_u8(str.val[0], vdupq_n_u8('z'));
	s2mask.val[1] = vcgeq_u8(str.val[1], vdupq_n_u8('a')) & vcleq_u8(str.val[1], vdupq_n_u8('z'));
	s2mask.val[2] = vcgeq_u8(str.val[2], vdupq_n_u8('a')) & vcleq_u8(str.val[2], vdupq_n_u8('z'));
	s2mask.val[3] = vcgeq_u8(str.val[3], vdupq_n_u8('a')) & vcleq_u8(str.val[3], vdupq_n_u8('z'));

	/* Set 3: "0123456789" */
	s3mask.val[0] = vcgeq_u8(str.val[0], vdupq_n_u8('0')) & vcleq_u8(str.val[0], vdupq_n_u8('9'));
	s3mask.val[1] = vcgeq_u8(str.val[1], vdupq_n_u8('0')) & vcleq_u8(str.val[1], vdupq_n_u8('9'));
	s3mask.val[2] = vcgeq_u8(str.val[2], vdupq_n_u8('0')) & vcleq_u8(str.val[2], vdupq_n_u8('9'));
	s3mask.val[3] = vcgeq_u8(str.val[3], vdupq_n_u8('0')) & vcleq_u8(str.val[3], vdupq_n_u8('9'));

	/* Set 4: "+" */
	s4mask.val[0] = vceqq_u8(str.val[0], vdupq_n_u8('+'));
	s4mask.val[1] = vceqq_u8(str.val[1], vdupq_n_u8('+'));
	s4mask.val[2] = vceqq_u8(str.val[2], vdupq_n_u8('+'));
	s4mask.val[3] = vceqq_u8(str.val[3], vdupq_n_u8('+'));

	/* Set 5: "/" */
	s5mask.val[0] = vceqq_u8(str.val[0], vdupq_n_u8('/'));
	s5mask.val[1] = vceqq_u8(str.val[1], vdupq_n_u8('/'));
	s5mask.val[2] = vceqq_u8(str.val[2], vdupq_n_u8('/'));
	s5mask.val[3] = vceqq_u8(str.val[3], vdupq_n_u8('/'));

	/* Check if all bytes have been classified;
	 * else fall back on bytewise code to do error checking and reporting: */
	classified  = (s1mask.val[0] | s2mask.val[0] | s3mask.val[0] | s4mask.val[0] | s5mask.val[0]);
	classified &= (s1mask.val[1] | s2mask.val[1] | s3mask.val[1] | s4mask.val[1] | s5mask.val[1]);
	classified &= (s1mask.val[2] | s2mask.val[2] | s3mask.val[2] | s4mask.val[2] | s5mask.val[2]);
	classified &= (s1mask.val[3] | s2mask.val[3] | s3mask.val[3] | s4mask.val[3] | s5mask.val[3]);
	classified  = vmvnq_u8(classified);

	/* Perform a narrowing shift to make a 64-bit vector: */
	narrowed = (uint32x2_t)vshrn_n_u16(classified, 4);

	/* Extract both 32-bit halves; check that all bits are zero: */
	if (vget_lane_u32(narrowed, 0) != 0
	 || vget_lane_u32(narrowed, 1) != 0) {
		break;
	}
	/* Subtract sets from byte values: */
	res.val[0] = s1mask.val[0] & vsubq_u8(str.val[0], vdupq_n_u8('A'));
	res.val[1] = s1mask.val[1] & vsubq_u8(str.val[1], vdupq_n_u8('A'));
	res.val[2] = s1mask.val[2] & vsubq_u8(str.val[2], vdupq_n_u8('A'));
	res.val[3] = s1mask.val[3] & vsubq_u8(str.val[3], vdupq_n_u8('A'));

	res.val[0] = vbslq_u8(s2mask.val[0], vsubq_u8(str.val[0], vdupq_n_u8('a' - 26)), res.val[0]);
	res.val[1] = vbslq_u8(s2mask.val[1], vsubq_u8(str.val[1], vdupq_n_u8('a' - 26)), res.val[1]);
	res.val[2] = vbslq_u8(s2mask.val[2], vsubq_u8(str.val[2], vdupq_n_u8('a' - 26)), res.val[2]);
	res.val[3] = vbslq_u8(s2mask.val[3], vsubq_u8(str.val[3], vdupq_n_u8('a' - 26)), res.val[3]);

	res.val[0] = vbslq_u8(s3mask.val[0], vsubq_u8(str.val[0], vdupq_n_u8('0' - 52)), res.val[0]);
	res.val[1] = vbslq_u8(s3mask.val[1], vsubq_u8(str.val[1], vdupq_n_u8('0' - 52)), res.val[1]);
	res.val[2] = vbslq_u8(s3mask.val[2], vsubq_u8(str.val[2], vdupq_n_u8('0' - 52)), res.val[2]);
	res.val[3] = vbslq_u8(s3mask.val[3], vsubq_u8(str.val[3], vdupq_n_u8('0' - 52)), res.val[3]);

	res.val[0] = vbslq_u8(s4mask.val[0], vdupq_n_u8(62), res.val[0]);
	res.val[1] = vbslq_u8(s4mask.val[1], vdupq_n_u8(62), res.val[1]);
	res.val[2] = vbslq_u8(s4mask.val[2], vdupq_n_u8(62), res.val[2]);
	res.val[3] = vbslq_u8(s4mask.val[3], vdupq_n_u8(62), res.val[3]);

	res.val[0] = vbslq_u8(s5mask.val[0], vdupq_n_u8(63), res.val[0]);
	res.val[1] = vbslq_u8(s5mask.val[1], vdupq_n_u8(63), res.val[1]);
	res.val[2] = vbslq_u8(s5mask.val[2], vdupq_n_u8(63), res.val[2]);
	res.val[3] = vbslq_u8(s5mask.val[3], vdupq_n_u8(63), res.val[3]);

	/* Compress four bytes into three: */
	dec.val[0] = vshlq_n_u8(res.val[0], 2) | vshrq_n_u8(res.val[1], 4);
	dec.val[1] = vshlq_n_u8(res.val[1], 4) | vshrq_n_u8(res.val[2], 2);
	dec.val[2] = vshlq_n_u8(res.val[2], 6) | res.val[3];

	/* Interleave and store decoded result: */
	vst3q_u8((uint8_t *)o, dec);

	c += 64;
	o += 48;
	outl += 48;
	srclen -= 64;
}
