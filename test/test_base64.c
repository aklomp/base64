#include <string.h>
#include <stdio.h>
#include "../include/libbase64.h"

static int fail = 0;
static char out[2000];
static size_t outlen;

extern char _binary_moby_dick_plain_txt_start[];
extern char _binary_moby_dick_plain_txt_end[];
extern char _binary_moby_dick_base64_txt_start[];
extern char _binary_moby_dick_base64_txt_end[];

static int
codec_supported (int flags)
{
	/* Check if given codec is supported by trying to decode a test string: */
	char *a = "aGVsbG8=";
	char b[10];
	size_t outlen;

	return (base64_decode(a, strlen(a), b, &outlen, flags) != -1);
}

static int
assert_enc_len (int flags, char *src, size_t srclen, char *dst, size_t dstlen)
{
	base64_encode(src, srclen, out, &outlen, flags);

	if (outlen != dstlen) {
		printf("FAIL: encoding of '%s': length expected %lu, got %lu\n", src,
			(unsigned long)dstlen,
			(unsigned long)outlen
		);
		fail = 1;
		return 0;
	}
	if (strncmp(dst, out, outlen) != 0) {
		out[outlen] = '\0';
		printf("FAIL: encoding of '%s': expected output '%s', got '%s'\n", src, dst, out);
		fail = 1;
		return 0;
	}
	return 1;
}

static int
assert_enc (int flags, char *src, char *dst)
{
	size_t srclen = strlen(src);
	size_t dstlen = strlen(dst);

	return assert_enc_len(flags, src, srclen, dst, dstlen);
}

static int
assert_dec_len (int flags, char *src, size_t srclen, char *dst, size_t dstlen)
{
	if (!base64_decode(src, srclen, out, &outlen, flags)) {
		printf("FAIL: decoding of '%s': decoding error\n", src);
		fail = 1;
		return 0;
	}
	if (outlen != dstlen) {
		printf("FAIL: encoding of '%s': "
			"length expected %lu, got %lu\n", src,
			(unsigned long)dstlen,
			(unsigned long)outlen
		);
		fail = 1;
		return 0;
	}
	if (strncmp(dst, out, outlen) != 0) {
		out[outlen] = '\0';
		printf("FAIL: decoding of '%s': expected output '%s', got '%s'\n", src, dst, out);
		fail = 1;
		return 0;
	}
	return 1;
}

static int
assert_dec (int flags, char *src, char *dst)
{
	size_t srclen = strlen(src);
	size_t dstlen = strlen(dst);

	return assert_dec_len(flags, src, srclen, dst, dstlen);
}

static int
assert_roundtrip (int flags, char *src)
{
	char tmp[100];
	size_t tmplen;
	size_t srclen = strlen(src);

	base64_encode(src, srclen, out, &outlen, flags);

	if (!base64_decode(out, outlen, tmp, &tmplen, flags)) {
		printf("FAIL: decoding of '%s': decoding error\n", out);
		fail = 1;
		return 0;
	}
	/* Check that 'src' is identical to 'tmp': */
	if (srclen != tmplen) {
		printf("FAIL: roundtrip of '%s': "
			"length expected %lu, got %lu\n", src,
			(unsigned long)srclen,
			(unsigned long)tmplen
		);
		fail = 1;
		return 0;
	}
	if (strncmp(src, tmp, tmplen) != 0) {
		tmp[tmplen] = '\0';
		printf("FAIL: roundtrip of '%s': got '%s'\n", src, tmp);
		fail = 1;
		return 0;
	}
	return 1;
}

static void
test_char_table (int flags)
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

		base64_encode(&chr[i], chrlen, enc, &enclen, BASE64_FORCE_PLAIN);

		if (!base64_decode(enc, enclen, dec, &declen, flags)) {
			printf("FAIL: decoding @ %d: decoding error\n", i);
			fail = 1;
			continue;
		}
		if (declen != chrlen) {
			printf("FAIL: roundtrip @ %d: "
				"length expected %lu, got %lu\n", i,
				(unsigned long)chrlen,
				(unsigned long)declen
			);
			fail = 1;
			continue;
		}
		if (strncmp(&chr[i], dec, declen) != 0) {
			printf("FAIL: roundtrip @ %d: decoded output not same as input\n", i);
			fail = 1;
		}
	}
}

static void
test_streaming (int flags)
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
	base64_encode(chr, 256, ref, &reflen, BASE64_FORCE_PLAIN);

	/* Encode the table with various block sizes and compare to reference: */
	for (bs = 1; bs < 255; bs++)
	{
		size_t inpos = 0;
		size_t partlen = 0;
		enclen = 0;

		base64_stream_encode_init(&state, flags);
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
			printf("FAIL: stream encoding gave incorrect size: "
				"%lu instead of %lu\n",
				(unsigned long)enclen,
				(unsigned long)reflen
			);
			fail = 1;
		}
		if (strncmp(ref, enc, reflen) != 0) {
			printf("FAIL: stream encoding with blocksize %lu failed\n",
				(unsigned long)bs
			);
			fail = 1;
		}
	}
	/* Decode the reference encoding with various block sizes and
	 * compare to input char table: */
	for (bs = 1; bs < 255; bs++)
	{
		size_t inpos = 0;
		size_t partlen = 0;
		enclen = 0;

		base64_stream_decode_init(&state, flags);
		memset(enc, 0, 400);
		while (base64_stream_decode(&state, &ref[inpos], (inpos + bs > reflen) ? reflen - inpos : bs, &enc[enclen], &partlen)) {
			enclen += partlen;
			inpos += bs;
		}
		if (enclen != 256) {
			printf("FAIL: stream decoding gave incorrect size: "
				"%lu instead of 255\n",
				(unsigned long)enclen
			);
			fail = 1;
		}
		if (strncmp(chr, enc, 256) != 0) {
			printf("FAIL: stream decoding with blocksize %lu failed\n",
				(unsigned long)bs
			);
			fail = 1;
		}
	}
}

int
main ()
{
	static char *codecs[] =
	{ "AVX2"
	, "NEON32"
	, "NEON64"
	, "plain"
	, "SSSE3"
	} ;
	unsigned int i, flags;
	int ret = 0;

	/* Loop over all codecs: */
	for (i = 0; i < sizeof(codecs) / sizeof(codecs[0]); i++)
	{
		fail = 0;
		flags = (1 << i);

		/* Is this codec supported? */
		if (!codec_supported(flags)) {
			printf("Codec %s:\n  skipping\n", codecs[i]);
			continue;
		}
		printf("Codec %s:\n", codecs[i]);

		/* These are the test vectors from RFC4648: */
		assert_enc(flags, "", "");
		assert_enc(flags, "f", "Zg==");
		assert_enc(flags, "fo", "Zm8=");
		assert_enc(flags, "foo", "Zm9v");
		assert_enc(flags, "foob", "Zm9vYg==");
		assert_enc(flags, "fooba", "Zm9vYmE=");
		assert_enc(flags, "foobar", "Zm9vYmFy");

		/* And their inverse: */
		assert_dec(flags, "", "");
		assert_dec(flags, "Zg==", "f");
		assert_dec(flags, "Zm8=", "fo");
		assert_dec(flags, "Zm9v", "foo");
		assert_dec(flags, "Zm9vYg==", "foob");
		assert_dec(flags, "Zm9vYmE=", "fooba");
		assert_dec(flags, "Zm9vYmFy", "foobar");

		/* The first paragraph from Moby Dick,
		 * to test the SIMD codecs with larger blocksize: */
		assert_enc_len(flags,
			_binary_moby_dick_plain_txt_start,
			_binary_moby_dick_plain_txt_end - _binary_moby_dick_plain_txt_start,
			_binary_moby_dick_base64_txt_start,
			_binary_moby_dick_base64_txt_end - _binary_moby_dick_base64_txt_start
		);
		assert_dec_len(flags,
			_binary_moby_dick_base64_txt_start,
			_binary_moby_dick_base64_txt_end - _binary_moby_dick_base64_txt_start,
			_binary_moby_dick_plain_txt_start,
			_binary_moby_dick_plain_txt_end - _binary_moby_dick_plain_txt_start
		);

		assert_roundtrip(flags, "");
		assert_roundtrip(flags, "f");
		assert_roundtrip(flags, "fo");
		assert_roundtrip(flags, "foo");
		assert_roundtrip(flags, "foob");
		assert_roundtrip(flags, "fooba");
		assert_roundtrip(flags, "foobar");

		assert_roundtrip(flags, "");
		assert_roundtrip(flags, "Zg==");
		assert_roundtrip(flags, "Zm8=");
		assert_roundtrip(flags, "Zm9v");
		assert_roundtrip(flags, "Zm9vYg==");
		assert_roundtrip(flags, "Zm9vYmE=");
		assert_roundtrip(flags, "Zm9vYmFy");

		test_char_table(flags);

		test_streaming(flags);

		if (fail == 0) {
			printf("  all tests passed.\n");
		}
		ret |= fail;
	}
	return ret;
}
