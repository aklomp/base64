// Translate 6-bit values 0..63 to the Base64 alphabet. There are five
// ranges, each with a constant offset from value to character:
//
//   #  From      To         Abs    Index  Characters
//   0  [0..25]   [65..90]   +65        0  ABCDEFGHIJKLMNOPQRSTUVWXYZ
//   1  [26..51]  [97..122]  +71     (0)   abcdefghijklmnopqrstuvwxyz
//   2  [52..61]  [48..57]    -4  [1..10]  0123456789
//   3  [62]      [43]       -19       11  +
//   4  [63]      [47]       -16       12  /
//
// Like the SSSE3/AVX2 codecs, this looks the offset up in a 16-entry
// table using a saturating subtract as the index, then adds it to the
// input. Ranges #0 and #1 share index 0; the +6 difference between them
// is applied afterwards to everything above range #0, so the entries for
// ranges #2..#4 are stored 6 lower than their real offset. A 16-entry
// table always fits in one vector register (VLEN >= 128 is guaranteed by
// the V extension), so the lookup is independent of VLEN.
static const uint8_t base64_rvv_enc_lut[16] = {
	 65U, 246U, 246U, 246U, 246U, 246U, 246U, 246U,
	246U, 246U, 246U, 231U, 234U,   0U,   0U,   0U,
};

static BASE64_FORCE_INLINE vuint8m1_t
enc_translate_rvv (const vuint8m1_t in, const vuint8m1_t lut, const size_t vl)
{
	// Index is 0 for [0..51] and 1..12 for [52..63]:
	const vuint8m1_t idx = __riscv_vssubu_vx_u8m1(in, 51, vl);

	// Add the table offset:
	vuint8m1_t out = __riscv_vadd_vv_u8m1(in, __riscv_vrgather_vv_u8m1(lut, idx, vl), vl);

	// Add 6 to everything above range #0:
	const vbool8_t gt25 = __riscv_vmsgtu_vx_u8m1_b8(in, 25, vl);
	out = __riscv_vadd_vx_u8m1_mu(gt25, out, out, 6, vl);

	return out;
}

// Encode vl groups of three input bytes into 4*vl output bytes.
static BASE64_FORCE_INLINE void
enc_loop_rvv_inner (const uint8_t *s, uint8_t *o, const vuint8m1_t lut, const size_t vl)
{
	// Load 3*vl bytes and deinterleave into three byte streams:
	const vuint8m1x3_t in = __riscv_vlseg3e8_v_u8m1x3(s, vl);
	const vuint8m1_t a = __riscv_vget_v_u8m1x3_u8m1(in, 0);
	const vuint8m1_t b = __riscv_vget_v_u8m1x3_u8m1(in, 1);
	const vuint8m1_t c = __riscv_vget_v_u8m1x3_u8m1(in, 2);

	// Divide the bits of three input bytes over four output bytes:
	// in[0]  = a7 a6 a5 a4 a3 a2 a1 a0
	// in[1]  = b7 b6 b5 b4 b3 b2 b1 b0
	// in[2]  = c7 c6 c5 c4 c3 c2 c1 c0
	// out[0] = 00 00 a7 a6 a5 a4 a3 a2
	// out[1] = 00 00 a1 a0 b7 b6 b5 b4
	// out[2] = 00 00 b3 b2 b1 b0 c7 c6
	// out[3] = 00 00 c5 c4 c3 c2 c1 c0
	//
	// The middle two use a multiply-add to combine the shifted halves
	// (the bit fields do not overlap, so add equals or), which saves
	// a shift and a mask each compared to the shift/or/and sequence.
	const vuint8m1_t v0 = __riscv_vsrl_vx_u8m1(a, 2, vl);
	const vuint8m1_t v1 = __riscv_vmacc_vx_u8m1(__riscv_vsrl_vx_u8m1(b, 4, vl), 16, __riscv_vand_vx_u8m1(a, 0x03, vl), vl);
	const vuint8m1_t v2 = __riscv_vmacc_vx_u8m1(__riscv_vsrl_vx_u8m1(c, 6, vl), 4, __riscv_vand_vx_u8m1(b, 0x0F, vl), vl);
	const vuint8m1_t v3 = __riscv_vand_vx_u8m1(c, 0x3F, vl);

	// Translate the 6-bit values to the Base64 alphabet:
	const vuint8m1x4_t out = __riscv_vcreate_v_u8m1x4(
		enc_translate_rvv(v0, lut, vl),
		enc_translate_rvv(v1, lut, vl),
		enc_translate_rvv(v2, lut, vl),
		enc_translate_rvv(v3, lut, vl));

	// Interleave and store 4*vl bytes of output:
	__riscv_vsseg4e8_v_u8m1x4(o, out, vl);
}

static inline void
enc_loop_rvv (const uint8_t **s, size_t *slen, uint8_t **o, size_t *olen)
{
	// Number of complete 3-byte input groups:
	size_t groups = *slen / 3;

	if (groups == 0) {
		return;
	}

	*slen -= groups * 3;	// 3 bytes consumed per group
	*olen += groups * 4;	// 4 bytes produced per group

	// Load the offset table. Only entries 0..12 are ever gathered, so
	// the (tail-agnostic) lanes beyond 16 do not matter:
	const vuint8m1_t lut = __riscv_vle8_v_u8m1(base64_rvv_enc_lut, 16);

	// Process two full-width chunks per iteration while possible; the
	// two are independent, which gives the core more to overlap:
	const size_t vlmax = __riscv_vsetvlmax_e8m1();

	while (groups >= 2 * vlmax) {
		enc_loop_rvv_inner(*s, *o, lut, vlmax);
		enc_loop_rvv_inner(*s + 3 * vlmax, *o + 4 * vlmax, lut, vlmax);
		*s += 6 * vlmax;
		*o += 8 * vlmax;
		groups -= 2 * vlmax;
	}

	// Remaining groups, including a final partial-width chunk:
	while (groups > 0) {
		const size_t vl = __riscv_vsetvl_e8m1(groups);
		enc_loop_rvv_inner(*s, *o, lut, vl);
		*s += vl * 3;
		*o += vl * 4;
		groups -= vl;
	}
}
