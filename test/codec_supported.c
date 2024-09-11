#include <stdio.h>
#include <stdlib.h>
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
, "AVX512"
, NULL
} ;

char **codecs = _codecs;

int
codec_supported (size_t index)
{
	if (index >= (sizeof(_codecs) / sizeof(_codecs[0])) - 1) {
		return 0;
	}
	// Check if given codec is supported by trying to decode a test string:
	char *a = "aGVsbG8=";
	char b[10];
	size_t outlen;
	char envVariable[32];
	sprintf(envVariable, "BASE64_TEST_SKIP_%s", _codecs[index]);
	const char* envOverride = getenv(envVariable);
	if ((envOverride != NULL) && (strcmp(envOverride, "1") == 0)) {
		return 0;
	}
	int flags = 1 << index;
	return (base64_decode(a, strlen(a), b, &outlen, flags) != -1) ? flags : 0;
}
