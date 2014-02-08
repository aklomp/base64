#include <stddef.h>	/* size_t */
#include "base64.h"

static const char
base64_table_enc[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

/* In the lookup table below, note that the value for '=' (character 61) is
 * 254, not 255. This character is used for in-band signaling of the end of
 * the datastream, and we will use that later. The characters A-Z, a-z, 0-9
 * and + / are mapped to their "decoded" values. The other bytes all map to
 * the value 255, which flags them as "invalid input". */

static const unsigned char
base64_table_dec[] =
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
base64_stream_encode_init (struct base64_state *state)
{
	state->eof = 0;
	state->bytes = 0;
	state->carry = 0;
}

void
base64_stream_encode (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
	/* Assume that *out is large enough to contain the output.
	 * Theoretically it should be 4/3 the length of src. */
	const unsigned char *c = (unsigned char *)src;
	char *o = out;

	/* Use local temporaries to avoid cache thrashing: */
	size_t outl = 0;
	struct base64_state st;
	st.bytes = state->bytes;
	st.carry = state->carry;

	/* Turn three bytes into four 6-bit numbers: */
	/* in[0] = 00111111 */
	/* in[1] = 00112222 */
	/* in[2] = 00222233 */
	/* in[3] = 00333333 */

	/* Duff's device, a for() loop inside a switch() statement. Legal! */
	switch (st.bytes)
	{
		for (;;)
		{
		case 0:	if (srclen-- == 0) {
				break;
			}
			*o++ = base64_table_enc[*c >> 2];
			st.carry = (*c++ << 4) & 0x30;
			st.bytes++;
			outl += 1;

		case 1:	if (srclen-- == 0) {
				break;
			}
			*o++ = base64_table_enc[st.carry | (*c >> 4)];
			st.carry = (*c++ << 2) & 0x3C;
			st.bytes++;
			outl += 1;

		case 2:	if (srclen-- == 0) {
				break;
			}
			*o++ = base64_table_enc[st.carry | (*c >> 6)];
			*o++ = base64_table_enc[*c++ & 0x3F];
			st.bytes = 0;
			outl += 2;
		}
	}
	state->bytes = st.bytes;
	state->carry = st.carry;
	*outlen = outl;
}

void
base64_stream_encode_final (struct base64_state *state, char *const out, size_t *const outlen)
{
	char *o = out;

	if (state->bytes == 1) {
		*o++ = base64_table_enc[state->carry];
		*o++ = '=';
		*o++ = '=';
		*outlen = 3;
		return;
	}
	if (state->bytes == 2) {
		*o++ = base64_table_enc[state->carry];
		*o++ = '=';
		*outlen = 2;
		return;
	}
	*outlen = 0;
}

void
base64_stream_decode_init (struct base64_state *state)
{
	state->eof = 0;
	state->bytes = 0;
	state->carry = 0;
}

int
base64_stream_decode (struct base64_state *state, const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
	int ret = 0;
	const char *c = src;
	char *o = out;
	unsigned char q;

	/* Use local temporaries to avoid cache thrashing: */
	size_t outl = 0;
	struct base64_state st;
	st.eof = state->eof;
	st.bytes = state->bytes;
	st.carry = state->carry;

	/* If we previously saw an EOF or an invalid character, bail out: */
	if (st.eof) {
		*outlen = 0;
		return 0;
	}
	/* Turn four 6-bit numbers into three bytes: */
	/* out[0] = 11111122 */
	/* out[1] = 22223333 */
	/* out[2] = 33444444 */

	/* Duff's device again: */
	switch (st.bytes)
	{
		for (;;)
		{
		case 0:	if (srclen-- == 0) {
				ret = 1;
				break;
			}
			if ((q = base64_table_dec[(unsigned char)*c++]) >= 254) {
				st.eof = 1;
				/* Treat character '=' as invalid for byte 0: */
				break;
			}
			st.carry = q << 2;
			st.bytes++;

		case 1:	if (srclen-- == 0) {
				ret = 1;
				break;
			}
			if ((q = base64_table_dec[(unsigned char)*c++]) >= 254) {
				st.eof = 1;
				/* Treat character '=' as invalid for byte 1: */
				break;
			}
			*o++ = st.carry | (q >> 4);
			st.carry = q << 4;
			st.bytes++;
			outl++;

		case 2:	if (srclen-- == 0) {
				ret = 1;
				break;
			}
			if ((q = base64_table_dec[(unsigned char)*c++]) >= 254) {
				st.eof = 1;
				/* When q == 254, the input char is '='. Return 1 and EOF.
				 * Technically, should check if next byte is also '=', but never mind.
				 * When q == 255, the input char is invalid. Return 0 and EOF. */
				ret = (q == 254) ? 1 : 0;
				break;
			}
			*o++ = st.carry | (q >> 2);
			st.carry = q << 6;
			st.bytes++;
			outl++;

		case 3:	if (srclen-- == 0) {
				ret = 1;
				break;
			}
			if ((q = base64_table_dec[(unsigned char)*c++]) >= 254) {
				st.eof = 1;
				/* When q == 254, the input char is '='. Return 1 and EOF.
				 * When q == 255, the input char is invalid. Return 0 and EOF. */
				ret = (q == 254) ? 1 : 0;
				break;
			}
			*o++ = st.carry | q;
			st.carry = 0;
			st.bytes = 0;
			outl++;
		}
	}
	state->eof = st.eof;
	state->bytes = st.bytes;
	state->carry = st.carry;
	*outlen = outl;
	return ret;
}

void
base64_encode (const char *const src, size_t srclen, char *const out, size_t *const outlen)
{
	size_t s;
	size_t t;
	struct base64_state state;

	/* Init the stream reader: */
	base64_stream_encode_init(&state);

	/* Feed the whole string to the stream reader: */
	base64_stream_encode(&state, src, srclen, out, &s);

	/* Finalize the stream by writing trailer if any: */
	base64_stream_encode_final(&state, out + s, &t);

	/* Final output length is stream length plus tail: */
	*outlen = s + t;
}

int
base64_decode (const char *const src, size_t srclen, char *const out, size_t *outlen)
{
	struct base64_state state;

	base64_stream_decode_init(&state);
	return base64_stream_decode(&state, src, srclen, out, outlen);
}
