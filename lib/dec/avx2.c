// If we have AVX2 support, pick off 32 bytes at a time for as long as we can,
// but make sure that we quit before seeing any == markers at the end of the
// string. Also, because we write 8 zeroes at the end of the output, ensure
// that there are at least 11 valid bytes of input data remaining to close the
// gap. 32 + 2 + 11 = 45 bytes:
while (srclen >= 45)
{
	// Load string:
	__m256i str = _mm256_loadu_si256((__m256i *)c);

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

	const __m256i set1 = CMPEQ(str, '+');
	const __m256i set2 = CMPEQ(str, '/');
	const __m256i set3 = RANGE(str, '0', '9');
	const __m256i set4 = RANGE(str, 'A', 'Z');
	const __m256i set5 = RANGE(str, 'a', 'z');

	__m256i delta = REPLACE(set1, 19);
	delta = _mm256_or_si256(delta, REPLACE(set2,  16));
	delta = _mm256_or_si256(delta, REPLACE(set3,   4));
	delta = _mm256_or_si256(delta, REPLACE(set4, -65));
	delta = _mm256_or_si256(delta, REPLACE(set5, -71));

	// Check for invalid input: if any of the delta values are zero,
	// fall back on bytewise code to do error checking and reporting:
	if (_mm256_movemask_epi8(CMPEQ(delta, 0))) {
		break;
	}

	// Now simply add the delta values to the input:
	str = _mm256_add_epi8(str, delta);

	// Shuffle bytes to 32-bit bigendian:
	str = _mm256_bswap_epi32(str);

	// Mask in a single byte per shift:
	__m256i mask = _mm256_set1_epi32(0x3F000000);

	// Pack bytes together:
	__m256i res = _mm256_slli_epi32(_mm256_and_si256(str, mask), 2);
	mask = _mm256_srli_epi32(mask, 8);

	res = _mm256_or_si256(res, _mm256_slli_epi32(_mm256_and_si256(str, mask), 4));
	mask = _mm256_srli_epi32(mask, 8);

	res = _mm256_or_si256(res, _mm256_slli_epi32(_mm256_and_si256(str, mask), 6));
	mask = _mm256_srli_epi32(mask, 8);

	res = _mm256_or_si256(res, _mm256_slli_epi32(_mm256_and_si256(str, mask), 8));

	// As in AVX2 encoding, we have to shuffle and repack each 128-bit lane
	// separately due to the way _mm256_shuffle_epi8 works:
	__m128i l0 = _mm_shuffle_epi8(_mm256_extractf128_si256(res, 0),
		_mm_setr_epi8(
			 3,  2,  1,
			 7,  6,  5,
			11, 10,  9,
			15, 14, 13,
			-1, -1, -1, -1));

	__m128i l1 = _mm_shuffle_epi8(_mm256_extractf128_si256(res, 1),
		_mm_setr_epi8(
			 3,  2,  1,
			 7,  6,  5,
			11, 10,  9,
			15, 14, 13,
			-1, -1, -1, -1));

	// Store back:
	_mm_storeu_si128((__m128i *) &o[ 0], l0);
	_mm_storeu_si128((__m128i *) &o[12], l1);

	c += 32;
	o += 24;
	outl += 24;
	srclen -= 32;
}
