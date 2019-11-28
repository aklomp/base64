static inline uint8x16x4_t
enc_reshuffle (const uint8x16x3_t in)
{
	uint8x16x4_t out;

#if defined(__GNUC__) || defined(__clang__)

	// GCC and Clang support the following inline assembly syntax. This
	// inline assembly implements the exact same algorithm as the
	// intrinsics further down, but benchmarks show that the inline
	// assembly easily beats the intrinsics. Perhaps this is because the
	// inline assembly is well pipelined to avoid data dependencies.

	__asm__ (
		"vshr.u8 %q[o0], %q[i0], #2    \n\t"
		"vshr.u8 %q[o1], %q[i1], #2    \n\t"
		"vshr.u8 %q[o2], %q[i2], #4    \n\t"
		"vsli.8  %q[o1], %q[i0], #6    \n\t"
		"vsli.8  %q[o2], %q[i1], #4    \n\t"
		"vshl.u8 %q[o3], %q[i2], #2    \n\t"

		"vshr.u8 %q[o1], %q[o1], #2    \n\t"
		"vshr.u8 %q[o2], %q[o2], #2    \n\t"
		"vshr.u8 %q[o3], %q[o3], #2    \n\t"

		// Outputs:
		: [o0] "=&w" (out.val[0]),
		  [o1] "=&w" (out.val[1]),
		  [o2] "=&w" (out.val[2]),
		  [o3] "=&w" (out.val[3])

		// Inputs:
		: [i0] "w" (in.val[0]),
		  [i1] "w" (in.val[1]),
		  [i2] "w" (in.val[2])
	);
#else
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
#endif

	return out;
}
