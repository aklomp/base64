#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/libbase64.h"
#include "../lib/codecs.h"

static int
cpu_has_x86_extension (const char *codec)
{
#if (__x86_64__ || __i386__ || _M_X86 || _M_X64)
	int feature_level;

	feature_level = x86_get_cpu_feature_level();

	if (strcmp(codec, "SSSE3") == 0) {
		return feature_level >= X86_FEATURE_LEVEL_SSSE3;
	}
	if (strcmp(codec, "SSE41") == 0) {
		return feature_level >= X86_FEATURE_LEVEL_SSE41;
	}
	if (strcmp(codec, "SSE42") == 0) {
		return feature_level >= X86_FEATURE_LEVEL_SSE42;
	}
	if (strcmp(codec, "AVX") == 0) {
		return feature_level >= X86_FEATURE_LEVEL_AVX;
	}
	if (strcmp(codec, "AVX2") == 0) {
		return feature_level >= X86_FEATURE_LEVEL_AVX2;
	}
	if (strcmp(codec, "AVX512") == 0) {
		return feature_level >= X86_FEATURE_LEVEL_AVX512;
	}
	return 0;
#else
	(void)codec;
	return 0;
#endif
}

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

	// Early out if CPU extension is not present:
	if (!cpu_has_x86_extension(_codecs[index])) {
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
