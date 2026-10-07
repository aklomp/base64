#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/libbase64.h"
#include "../lib/env.h"
#include "codec_supported.h"
#include "moby_dick.h"
#ifdef _OPENMP
#include <omp.h>
#endif

static char out[2000];
static size_t outlen;

static bool
assert_enc (int flags, const char *src, const char *dst)
{
	size_t srclen = strlen(src);
	size_t dstlen = strlen(dst);

	base64_encode(src, srclen, out, &outlen, flags);

	if (outlen != dstlen) {
		printf("FAIL: encoding of '%s': length expected %lu, got %lu\n", src,
			(unsigned long)dstlen,
			(unsigned long)outlen
		);
		return true;
	}
	if (strncmp(dst, out, outlen) != 0) {
		out[outlen] = '\0';
		printf("FAIL: encoding of '%s': expected output '%s', got '%s'\n", src, dst, out);
		return true;
	}
	return false;
}

static bool
assert_dec (int flags, const char *src, const char *dst)
{
	size_t srclen = strlen(src);
	size_t dstlen = strlen(dst);

	if (!base64_decode(src, srclen, out, &outlen, flags)) {
		printf("FAIL: decoding of '%s': decoding error\n", src);
		return true;
	}
	if (outlen != dstlen) {
		printf("FAIL: encoding of '%s': "
			"length expected %lu, got %lu\n", src,
			(unsigned long)dstlen,
			(unsigned long)outlen
		);
		return true;
	}
	if (strncmp(dst, out, outlen) != 0) {
		out[outlen] = '\0';
		printf("FAIL: decoding of '%s': expected output '%s', got '%s'\n", src, dst, out);
		return true;
	}
	return false;
}

static bool
assert_dec_full (int expected, int flags, const char *src, const char *dst)
{
	size_t srclen = strlen(src);
	size_t dstlen = strlen(dst);

	int ret = base64_decode(src, srclen, out, &outlen, flags);

	if (expected)
	{
		if (ret <= 0) {
			printf("FAIL: decoding of '%s': decoding error\n", src);
			return true;
		}
		if (outlen != dstlen) {
			printf("FAIL: decoding of '%s': "
			       "length expected %lu, got %lu\n", src,
			       (unsigned long)dstlen,
			       (unsigned long)outlen
			       );
			return true;
		}
		if (strncmp(dst, out, outlen) != 0) {
			out[outlen] = '\0';
			printf("FAIL: decoding of '%s': expected output '%s', got '%s'\n", src, dst, out);
			return true;
		}
		for (size_t bs = 1; bs <= srclen; ++bs) {
			struct base64_state state;
			char const* tmpsrc = src;
			size_t tmpsrclen = srclen;
			char *tmpout = out;
			size_t tmpoutlen;
			outlen = 0;

			base64_stream_decode_init(&state, flags);
			for (size_t b = 0; b < ((srclen + (bs - 1)) / bs); ++b, tmpsrc += bs, tmpsrclen -= bs) {
				size_t tmpbs = (tmpsrclen > bs) ? bs : tmpsrclen;
				ret = base64_stream_decode(&state, tmpsrc, tmpbs, tmpout, &tmpoutlen);
				if (ret <= 0) {
					printf("FAIL: decoding of '%s': decoding by %lu error\n", src, (unsigned long)bs);
					return true;
				}
				tmpout += tmpoutlen;
				outlen += tmpoutlen;
			}
			ret = base64_stream_decode_final(&state);
			if (ret <= 0) {
				printf("FAIL: decoding of '%s': decoding by %lu error\n", src, (unsigned long)bs);
				return true;
			}
			if (outlen != dstlen) {
				printf("FAIL: decoding of '%s': "
				       "length expected %lu, got %lu\n", src,
				       (unsigned long)dstlen,
				       (unsigned long)outlen
				       );
				return true;
			}
			if (strncmp(dst, out, outlen) != 0) {
				out[outlen] = '\0';
				printf("FAIL: decoding of '%s': expected output '%s', got '%s'\n", src, dst, out);
				return true;
			}
		}
	}
	else {
		if (ret > 0) {
			printf("FAIL: decoding of '%s': decoding succeeded\n", src);
			return true;
		}
		for (size_t bs = 1; bs <= srclen; ++bs) {
			struct base64_state state;
			char const* tmpsrc = src;
			size_t tmpsrclen = srclen;

			base64_stream_decode_init(&state, flags);
			for (size_t b = 0; b < ((srclen + (bs - 1)) / bs); ++b, tmpsrc += bs, tmpsrclen -= bs) {
				size_t tmpbs = (tmpsrclen > bs) ? bs : tmpsrclen;
				ret = base64_stream_decode(&state, tmpsrc, tmpbs, out, &outlen);
				if (ret <= 0) {
					break;
				}
			}
			if (ret > 0) {
				ret = base64_stream_decode_final(&state);
			}
			if (ret > 0) {
				printf("FAIL: decoding of '%s': bad no decoding error\n", src);
				return true;
			}
		}
	}
	return false;
}

static int
assert_roundtrip (int flags, const char *src)
{
	char tmp[1500];
	size_t tmplen;
	size_t srclen = strlen(src);

	// Encode the input into global buffer:
	base64_encode(src, srclen, out, &outlen, flags);

	// Decode the global buffer into local temp buffer:
	if (!base64_decode(out, outlen, tmp, &tmplen, flags)) {
		printf("FAIL: decoding of '%s': decoding error\n", out);
		return true;
	}

	// Check that 'src' is identical to 'tmp':
	if (srclen != tmplen) {
		printf("FAIL: roundtrip of '%s': "
			"length expected %lu, got %lu\n", src,
			(unsigned long)srclen,
			(unsigned long)tmplen
		);
		return true;
	}
	if (strncmp(src, tmp, tmplen) != 0) {
		tmp[tmplen] = '\0';
		printf("FAIL: roundtrip of '%s': got '%s'\n", src, tmp);
		return true;
	}

	return false;
}

static int
test_char_table (int flags, bool use_malloc)
{
	bool fail = false;
	char chr[256];
	char enc[400], dec[400];
	size_t enclen, declen;

	// Fill array with all characters 0..255:
	for (int i = 0; i < 256; i++)
		chr[i] = (unsigned char)i;

	// Loop, using each char as a starting position to increase test coverage:
	for (int i = 0; i < 256; i++) {

		size_t chrlen = 256 - i;
		char* src = &chr[i];
		if (use_malloc) {
			src = malloc(chrlen); /* malloc/copy this so valgrind can find out-of-bound access */
			if (src == NULL) {
				printf(
					"FAIL: encoding @ %d: allocation of %lu bytes failed\n",
					i, (unsigned long)chrlen
				);
				fail = true;
				continue;
			}
			memcpy(src, &chr[i], chrlen);
		}

		base64_encode(src, chrlen, enc, &enclen, flags);
		if (use_malloc) {
			free(src);
		}

		if (!base64_decode(enc, enclen, dec, &declen, flags)) {
			printf("FAIL: decoding @ %d: decoding error\n", i);
			fail = true;
			continue;
		}
		if (declen != chrlen) {
			printf("FAIL: roundtrip @ %d: "
				"length expected %lu, got %lu\n", i,
				(unsigned long)chrlen,
				(unsigned long)declen
			);
			fail = true;
			continue;
		}
		if (strncmp(&chr[i], dec, declen) != 0) {
			printf("FAIL: roundtrip @ %d: decoded output not same as input\n", i);
			fail = true;
		}
	}

	return fail;
}


static int
test_streaming (int flags)
{
	bool fail = false;
	char chr[256];
	char ref[400], enc[400];
	size_t reflen;

	// Fill array with all characters 0..255:
	for (int i = 0; i < 256; i++)
		chr[i] = (unsigned char)i;

	// Create reference base64 encoding:
	base64_encode(chr, 256, ref, &reflen, BASE64_FORCE_PLAIN);

	// Encode the table with various block sizes and compare to reference:
	for (size_t bs = 1; bs < 255; bs++)
	{
		size_t inpos   = 0;
		size_t partlen = 0;
		size_t enclen  = 0;
		struct base64_state state;

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
			fail = true;
		}
		if (strncmp(ref, enc, reflen) != 0) {
			printf("FAIL: stream encoding with blocksize %lu failed\n",
				(unsigned long)bs
			);
			fail = true;
		}
	}

	// Decode the reference encoding with various block sizes and
	// compare to input char table:
	for (size_t bs = 1; bs < 255; bs++)
	{
		size_t inpos   = 0;
		size_t partlen = 0;
		size_t enclen  = 0;
		struct base64_state state;

		base64_stream_decode_init(&state, flags);
		memset(enc, 0, 400);
		for (size_t b = 0; b < ((reflen + (bs - 1)) / bs); ++b, inpos += bs) {
			size_t tmpbs = ((reflen - inpos) > bs) ? bs : reflen - inpos;
			if (base64_stream_decode(&state, &ref[inpos], tmpbs, &enc[enclen], &partlen) <=0) {
				printf(
				       "FAIL: stream decoding with blocksize %lu failed at block start %lu\n",
				       (unsigned long)bs,
				       (unsigned long)inpos
				);
				fail |= true;
				break;
			}
			enclen += partlen;
		}
		if (base64_stream_decode_final(&state) <= 0) {
			printf(
				"FAIL: final stream decoding with blocksize %lu failed\n",
				(unsigned long)bs			);
			fail |= true;
		}
		if (enclen != 256) {
			printf("FAIL: stream decoding gave incorrect size: "
				"%lu instead of 255\n",
				(unsigned long)enclen
			);
			fail = true;
		}
		if (strncmp(chr, enc, 256) != 0) {
			printf("FAIL: stream decoding with blocksize %lu failed\n",
				(unsigned long)bs
			);
			fail = true;
		}
	}

	return fail;
}

static int
test_invalid_dec_input (int flags)
{
	// Subset of invalid characters to cover all ranges
	static const char invalid_set[] = { '\0', -1, '!', '-', ';', '_', '|' };
	static const char* invalid_strings[] = {
		"Zm9vYg=",
		"Zm9vYg",
		"Zm9vY",
		"Zm9vYmF=Zm9v"
	};

	bool fail = false;
	char chr[256];
	char enc[400], dec[400];
	size_t enclen, declen;

	// Fill array with all characters 0..255:
	for (int i = 0; i < 256; i++)
		chr[i] = (unsigned char)i;

	// Create reference base64 encoding:
	base64_encode(chr, 256, enc, &enclen, BASE64_FORCE_PLAIN);

	// Test invalid strings returns error.
	for (size_t i = 0U; i < sizeof(invalid_strings) / sizeof(invalid_strings[0]); ++i) {
		if (base64_decode(invalid_strings[i], strlen(invalid_strings[i]), dec, &declen, flags)) {
			printf("FAIL: decoding invalid input \"%s\": no decoding error\n", invalid_strings[i]);
			fail = true;
		}
	}

	// Loop, corrupting each char to increase test coverage:
	for (size_t c = 0U; c < sizeof(invalid_set); ++c) {
		for (size_t i = 0U; i < enclen; i++) {
			char backup = enc[i];

			enc[i] = invalid_set[c];

			if (base64_decode(enc, enclen, dec, &declen, flags)) {
				printf("FAIL: decoding invalid input @ %d: no decoding error\n", (int)i);
				fail = true;
				enc[i] = backup;
				continue;
			}
			enc[i] = backup;
		}
	}

	// Loop, corrupting two chars to increase test coverage:
	for (size_t c = 0U; c < sizeof(invalid_set); ++c) {
		for (size_t i = 0U; i < enclen - 2U; i++) {
			char backup  = enc[i+0];
			char backup2 = enc[i+2];

			enc[i+0] = invalid_set[c];
			enc[i+2] = invalid_set[c];

			if (base64_decode(enc, enclen, dec, &declen, flags)) {
				printf("FAIL: decoding invalid input @ %d: no decoding error\n", (int)i);
				fail = true;
				enc[i+0] = backup;
				enc[i+2] = backup2;
				continue;
			}
			enc[i+0] = backup;
			enc[i+2] = backup2;
		}
	}

	return fail;
}

static int
test_canonical (int flags)
{
	bool fail = false;

	// Test vectors:
	struct {
		const char *in;
		const char *out;
		int success;
	} vec[] = {
		{"", "", 1},
		{"Zg==", "f", 1},
		{"Zm8=", "fo", 1},
		{"Zm9v", "foo", 1},
		{"Zh==", "f", 0},
		{"Zm9=", "fo", 0},
	};

	for (size_t i = 0; i < sizeof(vec) / sizeof(vec[0]); i++) {
		fail |= assert_dec_full(1, flags, vec[i].in, vec[i].out);
		fail |= assert_dec_full(vec[i].success, flags | BASE64_CANONICAL, vec[i].in, vec[i].out);
	}
	return fail;
}


static int
test_padded_option(int flags)
{
	bool fail = false;

	// Test vectors:
	struct {
		const char *in;
		const char *out;
	} vec[] = {
		{"", ""},
		{"YQ==", "a"},
		{"YQ", "a"},
		{"YWI=", "ab"},
		{"YWI", "ab"},
		{"YWJj", "abc"}
	};

	for (size_t i = 0; i < sizeof(vec) / sizeof(vec[0]); i++) {
		/* test with padded = false & validation */
		if (strchr(vec[i].in, '=') != NULL) {
			fail |= assert_dec_full(0, flags | BASE64_NO_PADDING, vec[i].in, vec[i].out);
			fail |= assert_dec_full(1, flags, vec[i].in, vec[i].out);
			fail |= assert_enc(flags, vec[i].out, vec[i].in);
		}
		else {
			fail |= assert_dec_full(1, flags | BASE64_NO_PADDING, vec[i].in, vec[i].out);
			fail |= assert_enc(flags | BASE64_NO_PADDING, vec[i].out, vec[i].in);
		}
	}

	return fail;
}

static int
test_openmp_limit(int flags)
{
	bool fail = false;
	size_t const encodedLen = BASE64_OMP_THRESHOLD + 2U * 4U;
	size_t decodedLen = (encodedLen / 4) * 3;
	size_t const mobyDickLen = strlen(moby_dick_base64) - 4;
	char* encoded = malloc(encodedLen);
	char* decoded = malloc(decodedLen);
	if ((encoded == NULL) || (decoded == NULL)) {
		puts("allocation failed");
		free(encoded);
		free(decoded);
		return true;
	}
	for (size_t i = 0; i < ((encodedLen + mobyDickLen - 1) / mobyDickLen); ++i) {
		size_t const len = (((i + 1) * mobyDickLen) < encodedLen) ? mobyDickLen : encodedLen - (i * mobyDickLen);
		memcpy(encoded + i * mobyDickLen, moby_dick_base64, len);
	}

	/* fully valid stream */
	encoded[(encodedLen/2) - 4] = 'Z';
	encoded[(encodedLen/2) - 3] = 'g';
	encoded[(encodedLen/2) - 2] = 'g';
	encoded[(encodedLen/2) - 1] = 'g';
	encoded[encodedLen - 4] = 'Z';
	encoded[encodedLen - 3] = 'g';
	encoded[encodedLen - 2] = '=';
	encoded[encodedLen - 1] = '=';

	if (base64_decode(encoded, encodedLen, decoded, &decodedLen, flags | BASE64_CANONICAL) <= 0) {
		puts("test_openmp_limit failed test 1");
		fail = true;
	}

	encoded[(encodedLen/2) - 2] = '=';
	encoded[(encodedLen/2) - 1] = '=';

	if (base64_decode(encoded, encodedLen, decoded, &decodedLen, flags) > 0) {
		puts("test_openmp_limit failed test 2");
		fail = true;
	}

	/* restore fully valid stream  */
	encoded[(encodedLen/2) - 2] = 'g'; /* valid */
	encoded[(encodedLen/2) - 1] = 'g'; /* valid */
	if (base64_decode(encoded, encodedLen - 2, decoded, &decodedLen, flags | BASE64_NO_PADDING | BASE64_CANONICAL) <= 0) {
		puts("test_openmp_limit failed test 3");
		fail = true;
	}

	/* not canonical */
	encoded[encodedLen - 3] = 'h';
	if (base64_decode(encoded, encodedLen, decoded, &decodedLen, flags) <= 0) {
		puts("test_openmp_limit failed test 4");
		fail = true;
	}
	if (base64_decode(encoded, encodedLen, decoded, &decodedLen, flags | BASE64_CANONICAL) > 0) {
		puts("test_openmp_limit failed test 5");
		fail = true;
	}
	if (base64_decode(encoded, encodedLen, decoded, &decodedLen, flags | BASE64_NO_PADDING) > 0) {
		puts("test_openmp_limit failed test 6");
		fail = true;
	}
	if (base64_decode(encoded, encodedLen - 2, decoded, &decodedLen, flags | BASE64_NO_PADDING) <= 0) {
		puts("test_openmp_limit failed test 7");
		fail = true;
	}
	if (base64_decode(encoded, encodedLen - 2, decoded, &decodedLen, flags | BASE64_NO_PADDING | BASE64_CANONICAL) > 0) {
		puts("test_openmp_limit failed test 8");
		fail = true;
	}

	free(encoded);
	free(decoded);
	return fail;
}

static int
test_one_codec (size_t codec_index)
{
	bool fail = false;
	const char *codec = codecs[codec_index];

	printf("Codec %s:\n", codec);

	// Skip if this codec is not supported:
	int flags = codec_supported(codec_index);
	if (flags == 0) {
		puts("  skipping");
		return false;
	}

	// Test vectors:
	struct {
		const char *in;
		const char *out;
	} vec[] = {

		// These are the test vectors from RFC4648:
		{ "",		""         },
		{ "f",		"Zg=="     },
		{ "fo",		"Zm8="     },
		{ "foo",	"Zm9v"     },
		{ "foob",	"Zm9vYg==" },
		{ "fooba",	"Zm9vYmE=" },
		{ "foobar",	"Zm9vYmFy" },

		// The first paragraph from Moby Dick,
		// to test the SIMD codecs with larger blocksize:
		{ moby_dick_plain, moby_dick_base64 },
	};

	for (size_t i = 0; i < sizeof(vec) / sizeof(vec[0]); i++) {

		// Encode plain string, check against output:
		fail |= assert_enc(flags, vec[i].in, vec[i].out);

		// Decode the output string, check if we get the input:
		fail |= assert_dec(flags, vec[i].out, vec[i].in);

		// Do a roundtrip on the inputs and the outputs:
		fail |= assert_roundtrip(flags, vec[i].in);
		fail |= assert_roundtrip(flags, vec[i].out);
	}

	fail |= test_char_table(flags, false); /* test with unaligned input buffer */
	fail |= test_char_table(flags, true); /* test for out-of-bound input read */
	fail |= test_streaming(flags);
	fail |= test_invalid_dec_input(flags);
	fail |= test_canonical(flags);
	fail |= test_padded_option(flags);
	fail |= test_openmp_limit(flags);

	if (!fail)
		puts("  all tests passed.");

	return fail;
}

int
main ()
{
	bool fail = false;

#ifdef _OPENMP
	if (omp_get_max_threads() >= 2) {
		omp_set_num_threads(2);
	}
#endif

	// Loop over all codecs:
	for (size_t i = 0; codecs[i]; i++) {
		// Test this codec, merge the results:
		fail |= test_one_codec(i);
	}

	return (fail) ? 1 : 0;
}
