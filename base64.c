#include <stddef.h>	/* size_t */
#include "base64.h"

static int eof_decode = 0;
static int bytes_encode = 0;
static int bytes_decode = 0;
static unsigned char carry_encode = 0;
static unsigned char carry_decode = 0;

static char encode_lut[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

/* In the lookup table below, note that the value for '=' (character 61) is
 * 254, not 255. This character is used for in-band signaling of the end of
 * the datastream, and we will use that later. The characters A-Z, a-z, 0-9
 * and + / are mapped to their "decoded" values. The other bytes all map to
 * the value 255, which flags them as "invalid input". */

static unsigned char decode_lut[] =
{
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,		/*   0..15 */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,		/*  16..31 */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,		/*  32..47 */
	 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 254, 255, 255,		/*  48..63 */
	255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,		/*  64..79 */
	 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,		/*  80..95 */
	255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,		/*  96..111 */
	 41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,		/* 112..127 */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,		/* 128..143 */
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};

void
base64_encode (const char *const src, size_t srclen, char *const out, size_t *outlen)
{
	size_t s;
	size_t t;

	/* Init the stream reader: */
	base64_stream_encode_init();

	/* Feed the whole string to the stream reader: */
	base64_stream_encode(src, srclen, out, &s);

	/* Finalize the stream by writing trailer if any: */
	base64_stream_encode_final(out + s, &t);

	/* Final output length is stream length plus tail: */
	*outlen = s + t;
}

void
base64_stream_encode_init (void)
{
	bytes_encode = 0;
	carry_encode = 0;
}

void
base64_stream_encode (const char *const src, size_t srclen, char *const out, size_t *outlen)
{
	/* Assume that *out is large enough to contain the output.
	 * Theoretically it should be 4/3 the length of src. */
	const unsigned char *c = (unsigned char *)src;
	char *o = out;

	*outlen = 0;

	/* Turn three bytes into four 6-bit numbers: */
	/* in[0] = 00111111 */
	/* in[1] = 00112222 */
	/* in[2] = 00222233 */
	/* in[3] = 00333333 */

	/* Duff's device, a for() loop inside a switch() statement. Legal! */
	switch (bytes_encode)
	{
		for (;;)
		{
		case 0:	if (srclen-- == 0) {
				return;
			}
			*o++ = encode_lut[*c >> 2];
			carry_encode = (*c++ << 4) & 0x30;
			bytes_encode++;
			*outlen += 1;

		case 1:	if (srclen-- == 0) {
				return;
			}
			*o++ = encode_lut[carry_encode | (*c >> 4)];
			carry_encode = (*c++ << 2) & 0x3C;
			bytes_encode++;
			*outlen += 1;

		case 2:	if (srclen-- == 0) {
				return;
			}
			*o++ = encode_lut[carry_encode | (*c >> 6)];
			*o++ = encode_lut[*c++ & 0x3F];
			bytes_encode = 0;
			*outlen += 2;
		}
	}
}

void
base64_stream_encode_final (char *const out, size_t *outlen)
{
	char *o = out;

	if (bytes_encode == 1) {
		*o++ = encode_lut[carry_encode];
		*o++ = '=';
		*o++ = '=';
		*outlen = 3;
		return;
	}
	if (bytes_encode == 2) {
		*o++ = encode_lut[carry_encode];
		*o++ = '=';
		*outlen = 2;
		return;
	}
	*outlen = 0;
}

int
base64_decode (const char *const src, size_t srclen, char *const out, size_t *outlen)
{
	base64_stream_decode_init();
	return base64_stream_decode(src, srclen, out, outlen);
}

void
base64_stream_decode_init (void)
{
	eof_decode = 0;
	bytes_decode = 0;
	carry_decode = 0;
}

int
base64_stream_decode (const char *const src, size_t srclen, char *const out, size_t *outlen)
{
	const char *c = src;
	char *o = out;
	unsigned char q;

	*outlen = 0;

	/* If we previously saw an EOF or an invalid character, bail out: */
	if (eof_decode) {
		return 0;
	}
	/* Turn four 6-bit numbers into three bytes: */
	/* out[0] = 11111122 */
	/* out[1] = 22223333 */
	/* out[2] = 33444444 */

	/* Duff's device again: */
	switch (bytes_decode)
	{
		for (;;)
		{
		case 0:	/* First byte: */
			if (srclen-- == 0) {
				return 1;
			}
			if ((q = decode_lut[(unsigned char)*c++]) >= 254) {
				eof_decode = 1;
				/* Treat character '=' as invalid for byte 0: */
				return 0;
			}
			carry_decode = q << 2;
			bytes_decode++;

		case 1:	/* Second byte: */
			if (srclen-- == 0) {
				return 1;
			}
			if ((q = decode_lut[(unsigned char)*c++]) >= 254) {
				eof_decode = 1;
				/* Treat character '=' as invalid for byte 1: */
				return 0;
			}
			*o++ = carry_decode | (q >> 4);
			carry_decode = q << 4;
			bytes_decode++;
			(*outlen)++;

		case 2:	/* Third byte: */
			if (srclen-- == 0) {
				return 1;
			}
			if ((q = decode_lut[(unsigned char)*c++]) >= 254) {
				eof_decode = 1;
				/* When q == 254, the input char is '='. Return 1 and EOF.
				 * Technically, should check if next byte is also '=', but never mind.
				 * When q == 255, the input char is invalid. Return 0 and EOF. */
				return (q == 254);
			}
			*o++ = carry_decode | (q >> 2);
			carry_decode = q << 6;
			bytes_decode++;
			(*outlen)++;

		case 3:	/* Fourth byte: */
			if (srclen-- == 0) {
				return 1;
			}
			if ((q = decode_lut[(unsigned char)*c++]) >= 254) {
				eof_decode = 1;
				/* When q == 254, the input char is '='. Return 1 and EOF.
				 * When q == 255, the input char is invalid. Return 0 and EOF. */
				return (q == 254);
			}
			*o++ = carry_decode | q;
			carry_decode = 0;
			bytes_decode = 0;
			(*outlen)++;
		}
	}
	/* Never reached, but pacifies compiler: */
	return 0;
}
