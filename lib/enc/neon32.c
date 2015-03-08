/* If we have ARM NEON support, pick off 48 bytes at a time for as long as we can: */
while (srclen >= 48)
{
	uint8x16x3_t str;
	uint8x16x4_t res, hiset, himask;

	/* Load 48 bytes and deinterleave: */
	str = vld3q_u8((uint8_t *)c);

	/* Divide bits of three input bytes over four output bytes: */
	res.val[0] = vshrq_n_u8(str.val[0], 2);
	res.val[1] = vshrq_n_u8(str.val[1], 4) | vshlq_n_u8(str.val[0], 4);
	res.val[2] = vshrq_n_u8(str.val[2], 6) | vshlq_n_u8(str.val[1], 2);
	res.val[3] = str.val[2];

	/* Clear top two bits: */
	res.val[0] &= vdupq_n_u8(0x3F);
	res.val[1] &= vdupq_n_u8(0x3F);
	res.val[2] &= vdupq_n_u8(0x3F);
	res.val[3] &= vdupq_n_u8(0x3F);

	/* The bits have now been shifted to the right locations;
	 * translate their values 0..63 to the Base64 alphabet. */

	/* Make a mask for all bytes >= 32: */
	himask.val[0] = vcgeq_u8(res.val[0], vdupq_n_u8(32));
	himask.val[1] = vcgeq_u8(res.val[1], vdupq_n_u8(32));
	himask.val[2] = vcgeq_u8(res.val[2], vdupq_n_u8(32));
	himask.val[3] = vcgeq_u8(res.val[3], vdupq_n_u8(32));

	/* Make two sets; saturate-subtract 32 from high set: */
	hiset.val[0] = vqsubq_u8(res.val[0], vdupq_n_u8(32));
	hiset.val[1] = vqsubq_u8(res.val[1], vdupq_n_u8(32));
	hiset.val[2] = vqsubq_u8(res.val[2], vdupq_n_u8(32));
	hiset.val[3] = vqsubq_u8(res.val[3], vdupq_n_u8(32));

	/* Split sets into halves, do 32-byte table lookup on each half: */
	res.val[0] = vcombine_u8(
		vtbl4_u8(tbl_enc_lo, vget_low_u8(res.val[0])),
		vtbl4_u8(tbl_enc_lo, vget_high_u8(res.val[0]))
	);
	res.val[1] = vcombine_u8(
		vtbl4_u8(tbl_enc_lo, vget_low_u8(res.val[1])),
		vtbl4_u8(tbl_enc_lo, vget_high_u8(res.val[1]))
	);
	res.val[2] = vcombine_u8(
		vtbl4_u8(tbl_enc_lo, vget_low_u8(res.val[2])),
		vtbl4_u8(tbl_enc_lo, vget_high_u8(res.val[2]))
	);
	res.val[3] = vcombine_u8(
		vtbl4_u8(tbl_enc_lo, vget_low_u8(res.val[3])),
		vtbl4_u8(tbl_enc_lo, vget_high_u8(res.val[3]))
	);

	hiset.val[0] = vcombine_u8(
		vtbl4_u8(tbl_enc_hi, vget_low_u8(hiset.val[0])),
		vtbl4_u8(tbl_enc_hi, vget_high_u8(hiset.val[0]))
	);
	hiset.val[1] = vcombine_u8(
		vtbl4_u8(tbl_enc_hi, vget_low_u8(hiset.val[1])),
		vtbl4_u8(tbl_enc_hi, vget_high_u8(hiset.val[1]))
	);
	hiset.val[2] = vcombine_u8(
		vtbl4_u8(tbl_enc_hi, vget_low_u8(hiset.val[2])),
		vtbl4_u8(tbl_enc_hi, vget_high_u8(hiset.val[2]))
	);
	hiset.val[3] = vcombine_u8(
		vtbl4_u8(tbl_enc_hi, vget_low_u8(hiset.val[3])),
		vtbl4_u8(tbl_enc_hi, vget_high_u8(hiset.val[3]))
	);

	/* Construct the result: */
	res.val[0] = vbslq_u8(himask.val[0], hiset.val[0], res.val[0]);
	res.val[1] = vbslq_u8(himask.val[1], hiset.val[1], res.val[1]);
	res.val[2] = vbslq_u8(himask.val[2], hiset.val[2], res.val[2]);
	res.val[3] = vbslq_u8(himask.val[3], hiset.val[3], res.val[3]);

	/* Interleave and store result: */
	vst4q_u8((uint8_t *)o, res);

	c += 48;	/* 3 * 16 bytes of input  */
	o += 64;	/* 4 * 16 bytes of output */
	outl += 64;
	srclen -= 48;
}
