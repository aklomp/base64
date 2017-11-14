/* SSSE3 decode:

Use a Perfect Hash Function (PHF) to detect input bytes that are invalid Base64 
characters. Use a PHF to translate Base64 characters to the 6-bit value they represent.
Call dec_reshuffle to pack the 6-bit values together.

PSHUFB (_mm_shuffle_epi8) facilitates the PHF. It performs a vectorized table lookup 
using the low 4-bits of each byte as an index into a 16 byte table.  PSHUFB ignores 
bits 4, 5, and 6.  If bit 7 is set in the "index" byte then PSHUFB writes a zero to 
the result byte.

The high 4-bits of a Base64 char is shifted right to form part of the hash.  The shift 
operation fills bit 7 with garbage.

The low 4-bits of a Base64 char is used as an index into the association (asso) table. 
The asso values ensure there are no hash collisions. The asso values were found via 
guess and check.

PAVGB (_mm_avg_epu8) will sum the values, derived from the nibbles, and shift right 1 
bit. If the PAVGB temporary sum does not exceed 8-bits then bit 7 will be zero in the 
resultant hash value.

The delta_values table contains the value to be added 
to the input byte to decode it. It doesn't matter which 
delta_values is assigned to an invalid Base64 character.

delta_hash for 0 .. 7F is:
1 1 1 1 1 1 1 1 1 1 1 1 1 8 1 8
2 2 2 2 2 2 2 2 2 2 2 2 2 9 2 9
3 3 3 3 3 3 3 3 3 3 3 3 3 A 3 A
4 4 4 4 4 4 4 4 4 4 4 4 4 B 4 B
5 5 5 5 5 5 5 5 5 5 5 5 5 C 5 C
6 6 6 6 6 6 6 6 6 6 6 6 6 D 6 D
7 7 7 7 7 7 7 7 7 7 7 7 7 E 7 E
8 8 8 8 8 8 8 8 8 8 8 8 8 F 8 F
(note: bit 4, 5, and 6 are ignore by pshufb)

so the mapping is:
3 = plus (0x2B)
A = slash (0x2F)
B = equal sign (0x3D)
4 = numbers (0x30 .. 0x39)
5 6 C = upper case letters (0x41 .. 05A)
7 8 E = lower case letters (0x61 .. 0x7A)

uppercase letters use a delta of -65 (0xBF)
'A' (0x41) + 0xBF = 0x00
'Z' (0x5A) + 0xBF = 0x19

lowercase letters use a delta of -71 (0xB9)
'a' (0x61) + 0xB9 = 0x1A
'z' (0x7A) + 0xB9 = 0x33

numeric digits use a delta of 4 (0x04)
'0' (0x30) + 0x04 = 0x34
'9' (0x39) + 0x04 = 0x3D

plus sign uses a delta of 19 (0x13) 
'+' (0x2B) + 0x13 = 0x3E

forward slash uses a delta of 16 (0x10)
'/' (0x2F) + 0x10 = 0x3F

The check_values table contains the value to be added 
to the input byte to make it signed if it is not a valid
Base64 character.

check_hash for 0 .. 7F is:
7 1 1 1 1 1 1 1 1 1 2 4 6 6 6 8
8 2 2 2 2 2 2 2 2 2 3 5 7 7 7 9
9 3 3 3 3 3 3 3 3 3 4 6 8 8 8 A
A 4 4 4 4 4 4 4 4 4 5 7 9 9 9 B
B 5 5 5 5 5 5 5 5 5 6 8 A A A C
C 6 6 6 6 6 6 6 6 6 7 9 B B B D
D 7 7 7 7 7 7 7 7 7 8 A C C C E
E 8 8 8 8 8 8 8 8 8 9 B D D D F
(note: bit 4, 5, and 6 are ignore by pshufb)

so the mapping is:
0 1 2 3 B D F = invalid
4 = invalid below 31
5 = invalid below 41
6 = invalid below 2B
7 = invalid below 5A
8 = invalid below 4B
9 = invalid below 7A
A = invalid below 2F
C = invalid below 4F
E = invalid below 6F

'=' (0x3D) + 0x86 = 0xC3 // signed (bit_7 set)
'@' (0x40) + 0x80 = 0xC0 // signed
'A' (0x41) + 0xBF = 0x00 // unsigned (bit_7 clear)
'B' (0x42) + 0xBF = 0x01 // unsigned

for 80 .. FF (signed input):
All values in the check_values table are signed,  but it is possible
for the check_hash value to be signed in which case a zero value is returned by
PSHUFB.

So all signed input will remain signed because:
_mm_adds_epi8(signed, signed) = signed
_mm_adds_epi8(0, signed) = signed
*/


// If we have SSSE3 support, pick off 16 bytes at a time for as long as we can,
// but make sure that we quit before seeing any == markers at the end of the
// string. Also, because we write four zeroes at the end of the output, ensure
// that there are at least 6 valid bytes of input data remaining to close the
// gap. 16 + 2 + 6 = 24 bytes:
while (srclen >= 24)
{
	const __m128i delta_asso = _mm_setr_epi8(
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F
	);
	const __m128i delta_values = _mm_setr_epi8(
		0x00, 0x00, 0x00, 0x13, 0x04, 0xBF, 0xBF, 0xB9,
		0xB9, 0x00, 0x10, 0xC3, 0xBF, 0xBF, 0xB9, 0xB9
	);
	const __m128i check_asso = _mm_setr_epi8(
		0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x03, 0x07, 0x0B, 0x0B, 0x0B, 0x0F
	);
	const __m128i check_values = _mm_setr_epi8(
		0x80, 0x80, 0x80, 0x80, 0xCF, 0xBF, 0xD5, 0xA6,
		0xB5, 0x86, 0xD1, 0x80, 0xB1, 0x80, 0x91, 0x80
	);

	// Load string:
	__m128i asrc = _mm_loadu_si128((__m128i *)c);

	// Map input bytes to 6-bit values:
	const __m128i shifted = _mm_srli_epi32(asrc, 3);
	const __m128i delta_hash = _mm_avg_epu8(_mm_shuffle_epi8(delta_asso, asrc), shifted);
	__m128i out = _mm_add_epi8(_mm_shuffle_epi8(delta_values, delta_hash), asrc);

	// Detect invalid input bytes:
	const __m128i check_hash = _mm_avg_epu8(_mm_shuffle_epi8(check_asso, asrc), shifted);
	const __m128i chk = _mm_adds_epi8(_mm_shuffle_epi8(check_values, check_hash), asrc);

	// Fall back on bytewise code to do error checking and reporting:
	if (_mm_movemask_epi8(chk) != 0) {
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
bool Test_SSSE3_Decode()
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

	const __m128i delta_asso = _mm_setr_epi8(
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F
	);
	const __m128i delta_values = _mm_setr_epi8(
		0x00, 0x00, 0x00, 0x13, 0x04, 0xBF, 0xBF, 0xB9,
		0xB9, 0x00, 0x10, 0xC3, 0xBF, 0xBF, 0xB9, 0xB9
	);
	const __m128i check_asso = _mm_setr_epi8(
		0x0D, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x03, 0x07, 0x0B, 0x0B, 0x0B, 0x0F
	);
	const __m128i check_values = _mm_setr_epi8(
		0x80, 0x80, 0x80, 0x80, 0xCF, 0xBF, 0xD5, 0xA6,
		0xB5, 0x86, 0xD1, 0x80, 0xB1, 0x80, 0x91, 0x80
	);

	// check all possible values of 16 bits
	for(int i = 0; i < 0x10000; i++)
	{
		__m128i asrc, shifted, delta_hash, check_hash, out, chk, res;
		int r;

		asrc = _mm_cvtsi32_si128(i);

		shifted = _mm_srli_epi32(asrc, 3);
		delta_hash = _mm_avg_epu8(_mm_shuffle_epi8(delta_asso, asrc), shifted);
		out = _mm_add_epi8(_mm_shuffle_epi8(delta_values, delta_hash), asrc);

		check_hash = _mm_avg_epu8(_mm_shuffle_epi8(check_asso, asrc), shifted);		
		chk = _mm_adds_epi8(_mm_shuffle_epi8(check_values, check_hash), asrc);
		
		// set signed bytes to 255
		res = _mm_cmpgt_epi8(_mm_setzero_si128(), chk);
		res = _mm_or_si128(res, out);

		// get low 16 bits
		r = _mm_cvtsi128_si32(res) & 0x0000FFFF;
		
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
