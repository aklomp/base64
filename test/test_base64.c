#include <string.h>
#include <stdio.h>
#include "../base64.h"

static int ret = 0;
static char out[100];
static size_t outlen;

static int
assert_enc (char *src, char *dst)
{
	size_t srclen = strlen(src);
	size_t dstlen = strlen(dst);

	base64_encode(src, srclen, out, &outlen);

	if (outlen != dstlen) {
		printf("FAIL: encoding of '%s': length expected %lu, got %lu\n", src, dstlen, outlen);
		ret = 1;
		return 0;
	}
	if (strncmp(dst, out, outlen) != 0) {
		out[outlen] = '\0';
		printf("FAIL: encoding of '%s': expected output '%s', got '%s'\n", src, dst, out);
		ret = 1;
		return 0;
	}
	return 1;
}

static int
assert_dec (char *src, char *dst)
{
	size_t srclen = strlen(src);
	size_t dstlen = strlen(dst);

	if (!base64_decode(src, srclen, out, &outlen)) {
		printf("FAIL: decoding of '%s': decoding error\n", src);
		ret = 1;
		return 0;
	}
	if (outlen != dstlen) {
		printf("FAIL: encoding of '%s': length expected %lu, got %lu\n", src, dstlen, outlen);
		ret = 1;
		return 0;
	}
	if (strncmp(dst, out, outlen) != 0) {
		out[outlen] = '\0';
		printf("FAIL: decoding of '%s': expected output '%s', got '%s'\n", src, dst, out);
		ret = 1;
		return 0;
	}
	return 1;
}

static int
assert_roundtrip (char *src)
{
	char tmp[100];
	size_t tmplen;
	size_t srclen = strlen(src);

	base64_encode(src, srclen, out, &outlen);

	if (!base64_decode(out, outlen, tmp, &tmplen)) {
		printf("FAIL: decoding of '%s': decoding error\n", out);
		ret = 1;
		return 0;
	}
	/* Check that 'src' is identical to 'tmp': */
	if (srclen != tmplen) {
		printf("FAIL: roundtrip of '%s': length expected %lu, got %lu\n", src, srclen, tmplen);
		ret = 1;
		return 0;
	}
	if (strncmp(src, tmp, tmplen) != 0) {
		tmp[tmplen] = '\0';
		printf("FAIL: roundtrip of '%s': got '%s'\n", src, tmp);
		ret = 1;
		return 0;
	}
	return 1;
}

static void
test_char_table (void)
{
	int i;
	char chr[256];
	char enc[400], dec[400];
	size_t enclen, declen;

	/* Fill array with all characters 0..255: */
	for (i = 0; i < 256; i++) {
		chr[i] = (unsigned char)i;
	}
	/* Loop, using each char as a starting position to increase test coverage: */
	for (i = 0; i < 256; i++)
	{
		size_t chrlen = 256 - i;

		base64_encode(&chr[i], chrlen, enc, &enclen);

		if (!base64_decode(enc, enclen, dec, &declen)) {
			printf("FAIL: decoding @ %d: decoding error\n", i);
			ret = 1;
			continue;
		}
		if (declen != chrlen) {
			printf("FAIL: roundtrip @ %d: length expected %lu, got %lu\n", i, chrlen, declen);
			ret = 1;
			continue;
		}
		if (strncmp(&chr[i], dec, declen) != 0) {
			printf("FAIL: roundtrip @ %d: decoded output not same as input\n", i);
			ret = 1;
		}
	}
}

static void
test_streaming (void)
{
	int i;
	size_t bs;
	char chr[256];
	char ref[400], enc[400];
	size_t reflen, enclen;
	struct base64_state state;

	/* Fill array with all characters 0..255: */
	for (i = 0; i < 256; i++) {
		chr[i] = (unsigned char)i;
	}
	/* Create reference base64 encoding: */
	base64_encode(chr, 256, ref, &reflen);

	/* Encode the table with various block sizes and compare to reference: */
	for (bs = 1; bs < 255; bs++)
	{
		size_t inpos = 0;
		size_t partlen = 0;
		enclen = 0;

		base64_stream_encode_init(&state);
		memset(enc, 0, 400);
		for (;;) {
			base64_stream_encode(&state, &chr[inpos], (inpos + bs > 256) ? 256 - inpos : bs, &enc[enclen], &partlen);
			enclen += partlen;
			if (inpos + bs > 256) {
				break;
			}
			inpos += bs;
		}
		base64_stream_encode_final(&state, &enc[enclen], &partlen);
		enclen += partlen;

		if (enclen != reflen) {
			printf("FAIL: stream encoding gave incorrect size: %lu instead of %lu\n", enclen, reflen);
			ret = 1;
		}
		if (strncmp(ref, enc, reflen) != 0) {
			printf("FAIL: stream encoding with blocksize %lu failed\n", bs);
			ret = 1;
		}
	}
	/* Decode the reference encoding with various block sizes and
	 * compare to input char table: */
	for (bs = 1; bs < 255; bs++)
	{
		size_t inpos = 0;
		size_t partlen = 0;
		enclen = 0;

		base64_stream_decode_init(&state);
		memset(enc, 0, 400);
		while (base64_stream_decode(&state, &ref[inpos], (inpos + bs > reflen) ? reflen - inpos : bs, &enc[enclen], &partlen)) {
			enclen += partlen;
			inpos += bs;
		}
		if (enclen != 256) {
			printf("FAIL: stream decoding gave incorrect size: %lu instead of 255\n", enclen);
			ret = 1;
		}
		if (strncmp(chr, enc, 256) != 0) {
			printf("FAIL: stream decoding with blocksize %lu failed\n", bs);
			ret = 1;
		}
	}
}

int
main ()
{
	/* These are the test vectors from RFC4648: */
	assert_enc("", "");
	assert_enc("f", "Zg==");
	assert_enc("fo", "Zm8=");
	assert_enc("foo", "Zm9v");
	assert_enc("foob", "Zm9vYg==");
	assert_enc("fooba", "Zm9vYmE=");
	assert_enc("foobar", "Zm9vYmFy");

	/* And their inverse: */
	assert_dec("", "");
	assert_dec("Zg==", "f");
	assert_dec("Zm8=", "fo");
	assert_dec("Zm9v", "foo");
	assert_dec("Zm9vYg==", "foob");
	assert_dec("Zm9vYmE=", "fooba");
	assert_dec("Zm9vYmFy", "foobar");

	assert_roundtrip("");
	assert_roundtrip("f");
	assert_roundtrip("fo");
	assert_roundtrip("foo");
	assert_roundtrip("foob");
	assert_roundtrip("fooba");
	assert_roundtrip("foobar");

	assert_roundtrip("");
	assert_roundtrip("Zg==");
	assert_roundtrip("Zm8=");
	assert_roundtrip("Zm9v");
	assert_roundtrip("Zm9vYg==");
	assert_roundtrip("Zm9vYmE=");
	assert_roundtrip("Zm9vYmFy");

	test_char_table();

	test_streaming();

	if (ret == 0) fprintf(stderr, "All tests passed.\n");

	return ret;
}
