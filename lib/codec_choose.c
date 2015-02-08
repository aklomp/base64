#include <stddef.h>

#if __x86_64__ || __i386__
#include <cpuid.h>
#ifndef bit_SSSE3
#define bit_SSSE3 (1 << 9)
#endif
#endif

#include "../include/libbase64.h"
#include "codec_choose.h"
#include "config.h"

/* Function declarations: */
#define BASE64_CODEC_FUNCS(x)	\
	void base64_stream_encode_##x BASE64_ENC_PARAMS; \
	int  base64_stream_decode_##x BASE64_DEC_PARAMS;

BASE64_CODEC_FUNCS(avx2)
BASE64_CODEC_FUNCS(plain)
BASE64_CODEC_FUNCS(ssse3)

static int
codec_choose_x86 (struct codec *codec)
{
#if (__x86_64__ || __i386__) && HAVE_SSSE3

	unsigned int eax, ebx, ecx = 0, edx;
	unsigned int max_level = __get_cpuid_max(0, NULL);

	#if HAVE_SSSE3
	/* Check for SSSE3 support: */
	if (max_level >= 1) {
		__cpuid(1, eax, ebx, ecx, edx);
		if (ecx & bit_SSSE3) {
			codec->enc = base64_stream_encode_ssse3;
			codec->dec = base64_stream_decode_ssse3;
			return 1;
		}
	}
	#endif

#else
	(void)codec;
#endif

	return 0;
}

void
codec_choose (struct codec *codec)
{
	/* Runtime feature detection: */
	if (codec_choose_x86(codec)) {
		return;
	}
	codec->enc = base64_stream_encode_plain;
	codec->dec = base64_stream_decode_plain;
}
