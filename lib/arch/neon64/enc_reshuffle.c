static inline uint8x16x4_t
enc_reshuffle (const uint8x16x3_t in)
{
	uint8x16x4_t out;

	// Divide bits of three input bytes over four output bytes. All output
	// bytes except the first one are shifted over two bits to the left:
	out.val[0] = vshrq_n_u8(in.val[0], 2);
	out.val[1] = vshrq_n_u8(in.val[1], 2);
	out.val[2] = vshrq_n_u8(in.val[2], 4);
	out.val[1] = vsliq_n_u8(out.val[1], in.val[0], 6);
	out.val[2] = vsliq_n_u8(out.val[2], in.val[1], 4);
	out.val[3] = vshlq_n_u8(in.val[2], 2);

	// Clear the top two bits by shifting the output back to the right:
	out.val[1] = vshrq_n_u8(out.val[1], 2);
	out.val[2] = vshrq_n_u8(out.val[2], 2);
	out.val[3] = vshrq_n_u8(out.val[3], 2);

	return out;
}
