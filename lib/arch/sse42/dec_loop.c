// If we have SSE4.2 support, pick off 16 bytes at a time for as long as we can,
// but make sure that we quit before seeing any == markers at the end of the
// string. Also, because we write four zeroes at the end of the output, ensure
// that there are at least 6 valid bytes of input data remaining to close the
// gap. 16 + 2 + 6 = 24 bytes:
while (srclen >= 24)
{
	// Load string:
	__m128i str = _mm_loadu_si128((__m128i *)c);

	// The input consists of six character sets in the Base64 alphabet,
	// which we need to map back to the 6-bit values they represent.
	// There are three ranges, two singles, and then there's the rest.
	//
	//  #  From       To        Add    Index  Characters
	//  1  [43]       [62]      +19        0  +
	//  2  [47]       [63]      +16        1  /
	//  3  [48..57]   [52..61]   +4  [2..11]  0..9
	//  4  [65..90]   [0..25]   -65       15  A..Z
	//  5  [97..122]  [26..51]  -71       14  a..z
	// (6) Everything else => invalid input

	// LUT:
	const __m128i lut = _mm_setr_epi8(
		19, 16,   4,   4,
		 4,  4,   4,   4,
		 4,  4,   4,   4,
		 0,  0, -71, -65
	);

	// Ranges to be checked (all should be valid, repeat the first one):
	const __m128i range = _mm_setr_epi8(
		'+','+',
		'+','+',
		'+','+',
		'+','+',
		'/','/',
		'0','9',
		'A','Z',
		'a','z');

	// Check for invalid input:
	// pseudo-code for the _mm_cmpistrc call:
	// out_of_range = 0
	// for each byte of str
	//	out_of_range |= !(byte in one of the ranges)
	// return out_of_range
	if (_mm_cmpistrc(range, str, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_NEGATIVE_POLARITY)) {
		break;
	}

	// Compute indices for table look up:
	// First indices for ranges #[1..3]. Others are invalid.
	__m128i indices = _mm_subs_epu8(str, _mm_set1_epi8(46));

	// Compute mask for ranges #4 and #5:
	__m128i mask45 = CMPGT(str, 64);

	// Compute mask for range #5:
	__m128i mask5  = CMPGT(str, 96);

	// Clear invalid values in indices:
	indices = _mm_andnot_si128(mask45, indices);

	// Compute index for range #4: (abs(-1) << 4) + -1 = 15. Index for range #5 is off by one:
	mask45 = _mm_add_epi8(_mm_slli_epi16(_mm_abs_epi8(mask45), 4), mask45);

	// Set all indices. Index for range #5 is still off by one:
	indices = _mm_add_epi8(indices, mask45);

	// add -1, so substract 1 to indices for range #5, All indices are now correct:
	indices = _mm_add_epi8(indices, mask5);

	// Lookup deltas:
	__m128i delta = _mm_shuffle_epi8(lut, indices);

	// Now simply add the delta values to the input:
	str = _mm_add_epi8(str, delta);

	// Reshuffle the input to packed 12-byte output format:
	str = dec_reshuffle(str);

	// Store back:
	_mm_storeu_si128((__m128i *)o, str);

	c += 16;
	o += 12;
	outl += 12;
	srclen -= 16;
}
