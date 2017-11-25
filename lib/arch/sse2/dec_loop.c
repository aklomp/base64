// If we have SSE2 support, pick off 16 bytes at a time for as long as we can,
// but make sure that we quit before seeing any == markers at the end of the
// string. Also, because we write four zeroes at the end of the output, ensure
// that there are at least 6 valid bytes of input data remaining to close the
// gap. 16 + 2 + 6 = 24 bytes:
while (srclen >= 24)
{
	const __m128i digit_top     = _mm_set1_epi32( 0x46464646 ); // 0x46 = 0x7F - '9'
	const __m128i lowercase_top = _mm_set1_epi32( 0x05050505 ); // 0x05 = 0x7F - 'z'
	const __m128i uppercase_top = _mm_set1_epi32( 0x25252525 ); // 0x25 = 0x7F - 'Z'
	const __m128i digit_bottom  = _mm_set1_epi32( 0x76767676 ); // 0x76 = 0x7F - 9  ('9'-'0')
	const __m128i alpha_bottom  = _mm_set1_epi32( 0x66666666 ); // 0x66 = 0x7F - 25 ('Z'-'A') ('z'-'a') 
	const __m128i num_letters   = _mm_set1_epi32( 0x1A1A1A1A ); // 0x1A = 26
	const __m128i detect_plus   = _mm_set1_epi32( 0xFBFBFBFB ); // 0xFB = plus sign value after '0-9' roll down
	const __m128i adj_plus      = _mm_set1_epi32( 0xD4D4D4D4 ); // 0xD4 = plus sign shift after 'A-Z' roll down
	const __m128i detect_slash  = _mm_set1_epi32( 0xFFFFFFFF ); // 0xFF = after 0-9 roll down

	__m128i m0, m1, m2, m3, m4;

	// Load string:
	m0 = _mm_loadu_si128((__m128i *)c);

	// Map input bytes to 6-bit values:
	m1 = _mm_add_epi8(m0, digit_top); // "0-9" roll up
	m2 = _mm_add_epi8(m0, lowercase_top); // "a-z" roll up
	m1 = _mm_subs_epi8(m1, digit_bottom); // "0-9" roll down
	m2 = _mm_subs_epi8(m2, alpha_bottom); // "a-z" roll down
	m4 = _mm_cmpeq_epi8(m1, detect_slash); // match '/' ( foward slash )
	m3 = _mm_cmpeq_epi8(m1, detect_plus); // match '+' ( plus sign )
	m1 = _mm_adds_epu8(m1, num_letters); // ('0'=26 thru '9'=35) else signed
	m4 = _mm_subs_epu8(m4, uppercase_top); // 0xDA = 0xFF - 0x25
	m0 = _mm_add_epi8(m0, uppercase_top); // "A-Z" roll up
	m1 = _mm_xor_si128(m1, m4); // ( '/' = 37 ) = FF ^ DA
	m3 = _mm_and_si128(m3, adj_plus); // = 0xFF & 0xD4
	m0 = _mm_subs_epi8(m0, alpha_bottom); // "A-Z" roll down
	m1 = _mm_min_epu8(m1, m2); // merge
	m0 = _mm_xor_si128(m0, m3); // ( '+' = 62 )
	m1 = _mm_adds_epu8(m1, num_letters); //  ('a'=26 thru '9'=61, '/'=63) else signed
	m0 = _mm_min_epu8(m0, m1); // merge

	// Fall back on bytewise code to do error checking and reporting:
	unsigned int mask = _mm_movemask_epi8(m0);
	if(mask != 0){
		break;
	}

	// Pack 16 bytes into the 12-byte output format and store back:
	_mm_storeu_si128((__m128i *)o, dec_reshuffle(out));

	c += 16;
	o += 12;
	outl += 12;
	srclen -= 16;
}

/*
bool Test_SSE2_Decode()
{
	const unsigned char base64_table_dec[] =
	{
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
		 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 255, 255, 255,
		255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
		 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
		255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
		 41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	};

	const __m128i digit_top     = _mm_set1_epi32( 0x46464646 ); // 0x46 = 0x7F - '9'
	const __m128i lowercase_top = _mm_set1_epi32( 0x05050505 ); // 0x05 = 0x7F - 'z'
	const __m128i uppercase_top = _mm_set1_epi32( 0x25252525 ); // 0x25 = 0x7F - 'Z'
	const __m128i digit_bottom  = _mm_set1_epi32( 0x76767676 ); // 0x76 = 0x7F - 9  ('9'-'0')
	const __m128i alpha_bottom  = _mm_set1_epi32( 0x66666666 ); // 0x66 = 0x7F - 25 ('Z'-'A') ('z'-'a') 
	const __m128i num_letters   = _mm_set1_epi32( 0x1A1A1A1A ); // 0x1A = 26
	const __m128i detect_plus   = _mm_set1_epi32( 0xFBFBFBFB ); // 0xFB = plus sign value after '0-9' roll down
	const __m128i adj_plus      = _mm_set1_epi32( 0xD4D4D4D4 ); // 0xD4 = plus sign shift after 'A-Z' roll down
	const __m128i detect_slash  = _mm_set1_epi32( 0xFFFFFFFF ); // 0xFF = after 0-9 roll down

	// check all possible values of 16 bits
	for(int i = 0; i < 0x10000; i++)
	{
		__m128i m0, m1, m2, m3, m4;

		m0 = _mm_cvtsi32_si128(i); // load string

		m1 = _mm_add_epi8(m0, digit_top); // "0-9" roll up
		m2 = _mm_add_epi8(m0, lowercase_top); // "a-z" roll up
		m1 = _mm_subs_epi8(m1, digit_bottom); // "0-9" roll down
		m2 = _mm_subs_epi8(m2, alpha_bottom); // "a-z" roll down
		m4 = _mm_cmpeq_epi8(m1, detect_slash); // match '/' ( foward slash )
		m3 = _mm_cmpeq_epi8(m1, detect_plus); // match '+' ( plus sign )
		m1 = _mm_adds_epu8(m1, num_letters); // ('0'=26 thru '9'=35) else signed
		m4 = _mm_subs_epu8(m4, uppercase_top); // 0xDA = 0xFF - 0x25
		m0 = _mm_add_epi8(m0, uppercase_top); // "A-Z" roll up
		m1 = _mm_xor_si128(m1, m4); // ( '/' = 37 ) = FF ^ DA
		m3 = _mm_and_si128(m3, adj_plus); // = 0xFF & 0xD4
		m0 = _mm_subs_epi8(m0, alpha_bottom); // "A-Z" roll down
		m1 = _mm_min_epu8(m1, m2); // merge
		m0 = _mm_xor_si128(m0, m3); // ( '+' = 62 )
		m1 = _mm_adds_epu8(m1, num_letters); //  ('a'=26 thru '9'=61, '/'=63) else signed
		m0 = _mm_min_epu8(m0, m1); // merge
		
		// set signed bytes to 255
		__m128i res;
		res = _mm_cmpgt_epi8(_mm_setzero_si128(), m0);
		res = _mm_or_si128(res, m0);

		// get low 16 bits
		unsigned int r = _mm_cvtsi128_si32(res) & 0x0000FFFF;
		
		// check that byte_0 decoded correctly
		if((base64_table_dec[i & 0x00FF]) != (r & 0x00FF)){
			return false;
		}

		// check that byte_1 decoded correctly
		if((base64_table_dec[i >> 8]) != ((r >> 8) & 0x00FF)){
			return false;
		}
	}
	return true;
}
*/
