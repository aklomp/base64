#include <stddef.h>

#if __x86_64__ || __i386__
#include <cpuid.h>
#ifndef bit_AVX2
#define bit_AVX2 (1 << 5)
#endif
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
BASE64_CODEC_FUNCS(neon32)
BASE64_CODEC_FUNCS(neon64)
BASE64_CODEC_FUNCS(plain)
BASE64_CODEC_FUNCS(ssse3)

static int
codec_choose_arm (struct codec *codec)
{
#if (defined(__ARM_NEON__) || defined(__ARM_NEON)) && ((defined(__aarch64__) && HAVE_NEON64) || HAVE_NEON32)

	/* Unfortunately there is no portable way to check for NEON
	 * support at runtime from userland in the same way that x86
	 * has cpuid, so just stick to the compile-time configuration: */

	#if defined(__aarch64__) && HAVE_NEON64
	codec->enc = base64_stream_encode_neon64;
	codec->dec = base64_stream_decode_neon64;
	#else
	codec->enc = base64_stream_encode_neon32;
	codec->dec = base64_stream_decode_neon32;
	#endif

	return 1;

#else
	(void)codec;
	return 0;
#endif
}

static int
codec_choose_x86 (struct codec *codec)
{
#if (__x86_64__ || __i386__) && (HAVE_AVX2 || HAVE_SSSE3)

	unsigned int eax, ebx = 0, ecx = 0, edx;
	unsigned int max_level = __get_cpuid_max(0, NULL);

	#if HAVE_AVX2
	/* Check for AVX2 support: */
	if (max_level >= 7) {
		__cpuid_count(7, 0, eax, ebx, ecx, edx);
		if (ebx & bit_AVX2) {
			codec->enc = base64_stream_encode_avx2;
			codec->dec = base64_stream_decode_avx2;
			return 1;
		}
	}
	#endif

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
	if (codec_choose_arm(codec)) {
		return;
	}
	if (codec_choose_x86(codec)) {
		return;
	}
	codec->enc = base64_stream_encode_plain;
	codec->dec = base64_stream_decode_plain;
}
