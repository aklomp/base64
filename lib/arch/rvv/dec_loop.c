// Translate Base64 characters back to 6-bit values and detect invalid
// input. This uses the nibble-lookup scheme from the SSSE3/AVX2 codecs
// (due to Wojciech Mula), which needs only 16-entry tables and is
// therefore independent of VLEN.
//
// A character is valid iff (lut_lo[low nibble] & lut_hi[high nibble]) == 0.
// Each bit in these tables encodes one of the invalid character classes
// for that nibble position:
//
//   0x01: high nibble 2 (0x20..0x2F): everything except '+' and '/'
//   0x02: high nibble 3 (0x30..0x3F): everything except '0'..'9'
//   0x04: high nibble 4 or 6 (0x40, 0x60): '@' and '`'
//   0x08: high nibble 5 or 7 (0x5B.., 0x7B..): everything past 'Z' / 'z'
//   0x10: high nibble 0, 1 or 8..15: always invalid
static const uint8_t base64_rvv_dec_lut_lo[16] = {
	0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
	0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A,
};

static const uint8_t base64_rvv_dec_lut_hi[16] = {
	0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
};

// Offset to add to a valid character to get its 6-bit value, indexed by
// the high nibble, except that '/' (0x2F) uses index 1 instead of 2:
//
//   index  characters  offset
//   1      '/'         +16
//   2      '+'         +19
//   3      '0'..'9'    +4
//   4, 5   'A'..'Z'    -65
//   6, 7   'a'..'z'    -71
static const uint8_t base64_rvv_dec_lut_roll[16] = {
	0U, 16U, 19U, 4U, 191U, 191U, 185U, 185U,
	0U,  0U,  0U, 0U,   0U,   0U,   0U,   0U,
};

// Translate one stream of characters. The per-lane class byte is written
// to *invalid: nonzero means the character is not in the alphabet.
static BASE64_FORCE_INLINE vuint8m1_t
dec_translate_rvv (const vuint8m1_t str, const vuint8m1_t lut_lo,
	const vuint8m1_t lut_hi, const vuint8m1_t lut_roll, vuint8m1_t *invalid,
	const size_t vl)
{
	// Split each character into nibbles:
	const vuint8m1_t hi_nibbles = __riscv_vsrl_vx_u8m1(str, 4, vl);
	const vuint8m1_t lo_nibbles = __riscv_vand_vx_u8m1(str, 0x0F, vl);

	// Classify:
	const vuint8m1_t hi = __riscv_vrgather_vv_u8m1(lut_hi, hi_nibbles, vl);
	const vuint8m1_t lo = __riscv_vrgather_vv_u8m1(lut_lo, lo_nibbles, vl);
	*invalid = __riscv_vand_vv_u8m1(lo, hi, vl);

	// Roll index is the high nibble, minus one for '/':
	const vbool8_t eq_2F = __riscv_vmseq_vx_u8m1_b8(str, 0x2F, vl);
	const vuint8m1_t roll_idx = __riscv_vsub_vx_u8m1_mu(eq_2F, hi_nibbles, hi_nibbles, 1, vl);
	const vuint8m1_t roll = __riscv_vrgather_vv_u8m1(lut_roll, roll_idx, vl);

	return __riscv_vadd_vv_u8m1(str, roll, vl);
}

// Decode up to vl groups of four characters into 3*vl bytes. Returns the
// number of groups actually stored, which is less than vl if a group
// contains an invalid character (including '=' padding); that group and
// everything after it are left for the scalar code, which produces the
// proper error and padding handling.
static BASE64_FORCE_INLINE size_t
dec_loop_rvv_inner (const uint8_t *s, uint8_t *o, const vuint8m1_t lut_lo,
	const vuint8m1_t lut_hi, const vuint8m1_t lut_roll, size_t vl)
{
	// Load 4*vl bytes and deinterleave into four character streams:
	const vuint8m1x4_t in = __riscv_vlseg4e8_v_u8m1x4(s, vl);
	vuint8m1_t inv0, inv1, inv2, inv3;

	// Translate characters to 6-bit values:
	const vuint8m1_t v0 = dec_translate_rvv(__riscv_vget_v_u8m1x4_u8m1(in, 0), lut_lo, lut_hi, lut_roll, &inv0, vl);
	const vuint8m1_t v1 = dec_translate_rvv(__riscv_vget_v_u8m1x4_u8m1(in, 1), lut_lo, lut_hi, lut_roll, &inv1, vl);
	const vuint8m1_t v2 = dec_translate_rvv(__riscv_vget_v_u8m1x4_u8m1(in, 2), lut_lo, lut_hi, lut_roll, &inv2, vl);
	const vuint8m1_t v3 = dec_translate_rvv(__riscv_vget_v_u8m1x4_u8m1(in, 3), lut_lo, lut_hi, lut_roll, &inv3, vl);

	// Lane i of the combined class vector is nonzero if group i holds
	// an invalid character. Find the first such group:
	const vuint8m1_t inv = __riscv_vor_vv_u8m1(
		__riscv_vor_vv_u8m1(inv0, inv1, vl),
		__riscv_vor_vv_u8m1(inv2, inv3, vl), vl);
	const long first_invalid = __riscv_vfirst_m_b8(__riscv_vmsne_vx_u8m1_b8(inv, 0, vl), vl);

	if (first_invalid >= 0) {
		// Store only the valid groups before the bad one. The values
		// computed above remain correct for lanes below the reduced vl:
		vl = (size_t) first_invalid;
		if (vl == 0) {
			return 0;
		}
	}

	// Compress four 6-bit values into three bytes:
	// in[0]  = 00 a5 a4 a3 a2 a1 a0
	// in[1]  = 00 b5 b4 b3 b2 b1 b0
	// in[2]  = 00 c5 c4 c3 c2 c1 c0
	// in[3]  = 00 d5 d4 d3 d2 d1 d0
	// out[0] = a5 a4 a3 a2 a1 a0 b5 b4
	// out[1] = b3 b2 b1 b0 c5 c4 c3 c2
	// out[2] = c1 c0 d5 d4 d3 d2 d1 d0
	const vuint8m1x3_t out = __riscv_vcreate_v_u8m1x3(
		__riscv_vor_vv_u8m1(__riscv_vsll_vx_u8m1(v0, 2, vl), __riscv_vsrl_vx_u8m1(v1, 4, vl), vl),
		__riscv_vor_vv_u8m1(__riscv_vsll_vx_u8m1(v1, 4, vl), __riscv_vsrl_vx_u8m1(v2, 2, vl), vl),
		__riscv_vor_vv_u8m1(__riscv_vsll_vx_u8m1(v2, 6, vl), v3, vl));

	// Interleave and store 3*vl bytes of output:
	__riscv_vsseg3e8_v_u8m1x3(o, out, vl);

	return vl;
}

static inline void
dec_loop_rvv (const uint8_t **s, size_t *slen, uint8_t **o, size_t *olen)
{
	// Number of complete 4-byte input groups:
	size_t groups = *slen / 4;

	if (groups == 0) {
		return;
	}

	// Load the tables. Only indices 0..15 are ever gathered, so the
	// (tail-agnostic) lanes beyond 16 do not matter:
	const vuint8m1_t lut_lo   = __riscv_vle8_v_u8m1(base64_rvv_dec_lut_lo, 16);
	const vuint8m1_t lut_hi   = __riscv_vle8_v_u8m1(base64_rvv_dec_lut_hi, 16);
	const vuint8m1_t lut_roll = __riscv_vle8_v_u8m1(base64_rvv_dec_lut_roll, 16);

	// Process two full-width chunks per iteration while possible. The
	// second chunk only runs if the first one was entirely valid:
	const size_t vlmax = __riscv_vsetvlmax_e8m1();

	while (groups >= 2 * vlmax) {
		const size_t n0 = dec_loop_rvv_inner(*s, *o, lut_lo, lut_hi, lut_roll, vlmax);
		const size_t n1 = (n0 == vlmax)
			? dec_loop_rvv_inner(*s + 4 * vlmax, *o + 3 * vlmax, lut_lo, lut_hi, lut_roll, vlmax)
			: 0;
		const size_t n = n0 + n1;

		*s    += n * 4;
		*o    += n * 3;
		*slen -= n * 4;
		*olen += n * 3;
		groups -= n;

		if (n < 2 * vlmax) {
			return;
		}
	}

	// Remaining groups, including a final partial-width chunk:
	while (groups > 0) {
		const size_t vl = __riscv_vsetvl_e8m1(groups);
		const size_t n = dec_loop_rvv_inner(*s, *o, lut_lo, lut_hi, lut_roll, vl);

		*s    += n * 4;
		*o    += n * 3;
		*slen -= n * 4;
		*olen += n * 3;
		groups -= n;

		if (n < vl) {
			return;
		}
	}
}
