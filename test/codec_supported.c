#include <string.h>

#include "../include/libbase64.h"

static char *_codecs[] =
{ "AVX2"
, "NEON32"
, "NEON64"
, "plain"
, "SSSE3"
, "SSE41"
, "SSE42"
, "AVX"
, "SSSE3_ATOM"
, NULL
} ;

char **codecs = _codecs;

int
codec_supported (int flags)
{
	// Check if given codec is supported by trying to decode a test string:
	char *a = "aGVsbG8=";
	char b[10];
	size_t outlen;

	flags |= BASE64_CHECK_SUPPORT;
	return (base64_decode(a, strlen(a), b, &outlen, flags) != -1);
}
