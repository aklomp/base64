#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../include/libbase64.h"
#include "codecs.h"
#include "config.h"
#include "env.h"

#if (__x86_64__ || __i386__ || _M_X86 || _M_X64)
  #define BASE64_X86
  #if (HAVE_SSSE3 || HAVE_SSE41 || HAVE_SSE42 || HAVE_AVX || HAVE_AVX2 || HAVE_AVX512)
    #define BASE64_X86_SIMD
  #endif
#endif

// Function declarations:
#define BASE64_CODEC_FUNCS(arch)					\
	extern void base64_stream_encode_ ## arch BASE64_ENC_PARAMS;	\
	extern int  base64_stream_decode_ ## arch BASE64_DEC_PARAMS;

BASE64_CODEC_FUNCS(avx512)
BASE64_CODEC_FUNCS(avx2)
BASE64_CODEC_FUNCS(neon32)
BASE64_CODEC_FUNCS(neon64)
BASE64_CODEC_FUNCS(plain)
BASE64_CODEC_FUNCS(ssse3)
BASE64_CODEC_FUNCS(sse41)
BASE64_CODEC_FUNCS(sse42)
BASE64_CODEC_FUNCS(avx)

static bool
codec_choose_forced (struct codec *codec, int flags)
{
	// If the user wants to use a certain codec,
	// always allow it, even if the codec is a no-op.
	// For testing purposes.

	if (!(flags & 0xFFFF)) {
		return false;
	}

	if (flags & BASE64_FORCE_AVX2) {
		codec->enc = base64_stream_encode_avx2;
		codec->dec = base64_stream_decode_avx2;
		return true;
	}
	if (flags & BASE64_FORCE_NEON32) {
		codec->enc = base64_stream_encode_neon32;
		codec->dec = base64_stream_decode_neon32;
		return true;
	}
	if (flags & BASE64_FORCE_NEON64) {
		codec->enc = base64_stream_encode_neon64;
		codec->dec = base64_stream_decode_neon64;
		return true;
	}
	if (flags & BASE64_FORCE_PLAIN) {
		codec->enc = base64_stream_encode_plain;
		codec->dec = base64_stream_decode_plain;
		return true;
	}
	if (flags & BASE64_FORCE_SSSE3) {
		codec->enc = base64_stream_encode_ssse3;
		codec->dec = base64_stream_decode_ssse3;
		return true;
	}
	if (flags & BASE64_FORCE_SSE41) {
		codec->enc = base64_stream_encode_sse41;
		codec->dec = base64_stream_decode_sse41;
		return true;
	}
	if (flags & BASE64_FORCE_SSE42) {
		codec->enc = base64_stream_encode_sse42;
		codec->dec = base64_stream_decode_sse42;
		return true;
	}
	if (flags & BASE64_FORCE_AVX) {
		codec->enc = base64_stream_encode_avx;
		codec->dec = base64_stream_decode_avx;
		return true;
	}
	if (flags & BASE64_FORCE_AVX512) {
		codec->enc = base64_stream_encode_avx512;
		codec->dec = base64_stream_decode_avx512;
		return true;
	}
	return false;
}

static bool
codec_choose_arm (struct codec *codec)
{
#if HAVE_NEON64 || ((defined(__ARM_NEON__) || defined(__ARM_NEON)) && HAVE_NEON32)

	// Unfortunately there is no portable way to check for NEON
	// support at runtime from userland in the same way that x86
	// has cpuid, so just stick to the compile-time configuration:

	#if HAVE_NEON64
	codec->enc = base64_stream_encode_neon64;
	codec->dec = base64_stream_decode_neon64;
	#else
	codec->enc = base64_stream_encode_neon32;
	codec->dec = base64_stream_decode_neon32;
	#endif

	return true;

#else
	(void)codec;
	return false;
#endif
}

#ifdef BASE64_X86_SIMD

static const int cpu_feature_flags[X86_FEATURE_LEVEL_COUNT] = {
	[X86_FEATURE_LEVEL_NONE]   = 0,
	[X86_FEATURE_LEVEL_SSSE3]  = BASE64_FORCE_SSSE3,
	[X86_FEATURE_LEVEL_SSE41]  = BASE64_FORCE_SSE41,
	[X86_FEATURE_LEVEL_SSE42]  = BASE64_FORCE_SSE42,
	[X86_FEATURE_LEVEL_AVX]    = BASE64_FORCE_AVX,
	[X86_FEATURE_LEVEL_AVX2]   = BASE64_FORCE_AVX2,
	[X86_FEATURE_LEVEL_AVX512] = BASE64_FORCE_AVX512
};

#endif // BASE64_X86

static bool
codec_choose_x86 (struct codec *codec)
{
#ifdef BASE64_X86_SIMD
	int feature_level;

	feature_level = x86_get_cpu_feature_level();
	codec_choose_forced(codec, cpu_feature_flags[feature_level]);
	return true;

#else
	(void)codec;

	return false;
#endif // BASE64_X86_SIMD
}

void
codec_choose (struct codec *codec, int flags)
{
	// User forced a codec:
	if (codec_choose_forced(codec, flags)) {
		return;
	}

	// Runtime feature detection:
	if (codec_choose_arm(codec)) {
		return;
	}
	if (codec_choose_x86(codec)) {
		return;
	}
	codec->enc = base64_stream_encode_plain;
	codec->dec = base64_stream_decode_plain;
}
