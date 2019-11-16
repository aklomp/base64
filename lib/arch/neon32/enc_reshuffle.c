static inline uint8x16x4_t
enc_reshuffle (uint8x16x3_t in)
{
	uint8x16x4_t out;

	// Divide bits of three input bytes over four output bytes:
	out.val[0] = vshrq_n_u8(in.val[0], 2);
	out.val[1] = vorrq_u8(vshrq_n_u8(in.val[1], 4), vshlq_n_u8(in.val[0], 4));
	out.val[2] = vorrq_u8(vshrq_n_u8(in.val[2], 6), vshlq_n_u8(in.val[1], 2));
	out.val[3] = in.val[2];

	// Clear top two bits:
	out.val[0] = vandq_u8(out.val[0], vdupq_n_u8(0x3F));
	out.val[1] = vandq_u8(out.val[1], vdupq_n_u8(0x3F));
	out.val[2] = vandq_u8(out.val[2], vdupq_n_u8(0x3F));
	out.val[3] = vandq_u8(out.val[3], vdupq_n_u8(0x3F));

	return out;
}
