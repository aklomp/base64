// If we have NEON support on 32-bit ARM, pick off 64 bytes at a time for as
// long as we can. Unlike the SSE codecs, we don't write trailing zero bytes to
// output, so we don't need to check if we have enough remaining input to cover
// them:
while (srclen >= 64)
{
	uint8x16x4_t set1, set2, set3, set4, set5, delta;
	uint8x16x3_t dec;

	// Load 64 bytes and deinterleave:
	uint8x16x4_t str = vld4q_u8((uint8_t *)c);

	// The input consists of six character sets in the Base64 alphabet,
	// which we need to map back to the 6-bit values they represent.
	// There are three ranges, two singles, and then there's the rest.
	//
	//  #  From       To        Add  Characters
	//  1  [43]       [62]      +19  +
	//  2  [47]       [63]      +16  /
	//  3  [48..57]   [52..61]   +4  0..9
	//  4  [65..90]   [0..25]   -65  A..Z
	//  5  [97..122]  [26..51]  -71  a..z
	// (6) Everything else => invalid input

	set1.val[0] = CMPEQ(str.val[0], '+');
	set1.val[1] = CMPEQ(str.val[1], '+');
	set1.val[2] = CMPEQ(str.val[2], '+');
	set1.val[3] = CMPEQ(str.val[3], '+');

	set2.val[0] = CMPEQ(str.val[0], '/');
	set2.val[1] = CMPEQ(str.val[1], '/');
	set2.val[2] = CMPEQ(str.val[2], '/');
	set2.val[3] = CMPEQ(str.val[3], '/');

	set3.val[0] = RANGE(str.val[0], '0', '9');
	set3.val[1] = RANGE(str.val[1], '0', '9');
	set3.val[2] = RANGE(str.val[2], '0', '9');
	set3.val[3] = RANGE(str.val[3], '0', '9');

	set4.val[0] = RANGE(str.val[0], 'A', 'Z');
	set4.val[1] = RANGE(str.val[1], 'A', 'Z');
	set4.val[2] = RANGE(str.val[2], 'A', 'Z');
	set4.val[3] = RANGE(str.val[3], 'A', 'Z');

	set5.val[0] = RANGE(str.val[0], 'a', 'z');
	set5.val[1] = RANGE(str.val[1], 'a', 'z');
	set5.val[2] = RANGE(str.val[2], 'a', 'z');
	set5.val[3] = RANGE(str.val[3], 'a', 'z');

	delta.val[0] = REPLACE(set1.val[0], 19);
	delta.val[1] = REPLACE(set1.val[1], 19);
	delta.val[2] = REPLACE(set1.val[2], 19);
	delta.val[3] = REPLACE(set1.val[3], 19);

	delta.val[0] = vorrq_u8(delta.val[0], REPLACE(set2.val[0], 16));
	delta.val[1] = vorrq_u8(delta.val[1], REPLACE(set2.val[1], 16));
	delta.val[2] = vorrq_u8(delta.val[2], REPLACE(set2.val[2], 16));
	delta.val[3] = vorrq_u8(delta.val[3], REPLACE(set2.val[3], 16));

	delta.val[0] = vorrq_u8(delta.val[0], REPLACE(set3.val[0], 4));
	delta.val[1] = vorrq_u8(delta.val[1], REPLACE(set3.val[1], 4));
	delta.val[2] = vorrq_u8(delta.val[2], REPLACE(set3.val[2], 4));
	delta.val[3] = vorrq_u8(delta.val[3], REPLACE(set3.val[3], 4));

	delta.val[0] = vorrq_u8(delta.val[0], REPLACE(set4.val[0], -65));
	delta.val[1] = vorrq_u8(delta.val[1], REPLACE(set4.val[1], -65));
	delta.val[2] = vorrq_u8(delta.val[2], REPLACE(set4.val[2], -65));
	delta.val[3] = vorrq_u8(delta.val[3], REPLACE(set4.val[3], -65));

	delta.val[0] = vorrq_u8(delta.val[0], REPLACE(set5.val[0], -71));
	delta.val[1] = vorrq_u8(delta.val[1], REPLACE(set5.val[1], -71));
	delta.val[2] = vorrq_u8(delta.val[2], REPLACE(set5.val[2], -71));
	delta.val[3] = vorrq_u8(delta.val[3], REPLACE(set5.val[3], -71));

	// Check for invalid input: if any of the delta values are zero,
	// fall back on bytewise code to do error checking and reporting:
	uint8x16_t classified = CMPEQ(delta.val[0], 0);
	classified = vorrq_u8(classified, CMPEQ(delta.val[1], 0));
	classified = vorrq_u8(classified, CMPEQ(delta.val[2], 0));
	classified = vorrq_u8(classified, CMPEQ(delta.val[3], 0));

	// Extract both 32-bit halves; check that all bits are zero:
	if (vgetq_lane_u32((uint32x4_t)classified, 0) != 0
	 || vgetq_lane_u32((uint32x4_t)classified, 1) != 0
	 || vgetq_lane_u32((uint32x4_t)classified, 2) != 0
	 || vgetq_lane_u32((uint32x4_t)classified, 3) != 0) {
		break;
	}

	// Now simply add the delta values to the input:
	str.val[0] = vaddq_u8(str.val[0], delta.val[0]);
	str.val[1] = vaddq_u8(str.val[1], delta.val[1]);
	str.val[2] = vaddq_u8(str.val[2], delta.val[2]);
	str.val[3] = vaddq_u8(str.val[3], delta.val[3]);

	// Compress four bytes into three:
	dec.val[0] = vshlq_n_u8(str.val[0], 2) | vshrq_n_u8(str.val[1], 4);
	dec.val[1] = vshlq_n_u8(str.val[1], 4) | vshrq_n_u8(str.val[2], 2);
	dec.val[2] = vshlq_n_u8(str.val[2], 6) | str.val[3];

	// Interleave and store decoded result:
	vst3q_u8((uint8_t *)o, dec);

	c += 64;
	o += 48;
	outl += 48;
	srclen -= 64;
}
