// The RISC-V codec selection reaches riscv_hwprobe(2) through syscall(2),
// which glibc and musl only declare with this feature macro under -std=c99:
#if defined(__riscv) && defined(__linux__) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE
#endif

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

#if defined(__riscv) && HAVE_RVV && defined(__linux__)
  #define BASE64_RISCV_RVV
  #include <unistd.h>
  #include <sys/syscall.h>
  #include <sys/auxv.h>

  // The riscv_hwprobe(2) ABI is stable; define what is needed here so
  // that building does not depend on recent kernel headers.
  #ifndef __NR_riscv_hwprobe
  #define __NR_riscv_hwprobe 258
  #endif
  #define BASE64_RISCV_HWPROBE_KEY_IMA_EXT_0	4
  #define BASE64_RISCV_HWPROBE_IMA_V		(1 << 2)
  #define BASE64_RISCV_HWCAP_V			(1UL << ('V' - 'A'))

  struct base64_riscv_hwprobe {
	int64_t  key;
	uint64_t value;
  };
#endif

#ifdef BASE64_X86
#ifdef _MSC_VER
	#include <intrin.h>
	#define __cpuid_count(__level, __count, __eax, __ebx, __ecx, __edx) \
	{						\
		int info[4];				\
		__cpuidex(info, __level, __count);	\
		__eax = info[0];			\
		__ebx = info[1];			\
		__ecx = info[2];			\
		__edx = info[3];			\
	}
	#define __cpuid(__level, __eax, __ebx, __ecx, __edx) \
		__cpuid_count(__level, 0, __eax, __ebx, __ecx, __edx)
#else
	#include <cpuid.h>
	#if HAVE_AVX512 || HAVE_AVX2 || HAVE_AVX
		#if ((__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 2) || (__clang_major__ >= 3))
			static inline uint64_t _xgetbv (uint32_t index)
			{
				uint32_t eax, edx;
				__asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
				return ((uint64_t)edx << 32) | eax;
			}
		#else
			#error "Platform not supported"
		#endif
	#endif
#endif

#ifndef bit_AVX512vl
#define bit_AVX512vl (1 << 31)
#endif
#ifndef bit_AVX512vbmi
#define bit_AVX512vbmi (1 << 1)
#endif
#ifndef bit_AVX2
#define bit_AVX2 (1 << 5)
#endif
#ifndef bit_SSSE3
#define bit_SSSE3 (1 << 9)
#endif
#ifndef bit_SSE41
#define bit_SSE41 (1 << 19)
#endif
#ifndef bit_SSE42
#define bit_SSE42 (1 << 20)
#endif
#ifndef bit_AVX
#define bit_AVX (1 << 28)
#endif

#define bit_XSAVE_XRSTORE (1 << 27)

#ifndef _XCR_XFEATURE_ENABLED_MASK
#define _XCR_XFEATURE_ENABLED_MASK 0
#endif

#define bit_XMM      (1 << 1)
#define bit_YMM      (1 << 2)
#define bit_OPMASK   (1 << 5)
#define bit_ZMM      (1 << 6)
#define bit_HIGH_ZMM (1 << 7)

#define _XCR_XMM_AND_YMM_STATE_ENABLED_BY_OS (bit_XMM | bit_YMM)

#define _AVX_512_ENABLED_BY_OS (bit_XMM | bit_YMM | bit_OPMASK | bit_ZMM | bit_HIGH_ZMM)

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
BASE64_CODEC_FUNCS(rvv)

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
	if (flags & BASE64_FORCE_RVV) {
		codec->enc = base64_stream_encode_rvv;
		codec->dec = base64_stream_decode_rvv;
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

static bool
codec_choose_riscv (struct codec *codec)
{
#ifdef BASE64_RISCV_RVV

	// The RVV codec is compiled with the vector extension enabled and
	// uses vector instructions unconditionally, so it must only be
	// selected when the running CPU actually has the V extension.
	// Ask the kernel through riscv_hwprobe(2) (Linux 6.4+), and fall
	// back to the AT_HWCAP bit on older kernels:
	bool have_v = false;
	struct base64_riscv_hwprobe pair = {
		.key   = BASE64_RISCV_HWPROBE_KEY_IMA_EXT_0,
		.value = 0,
	};

	if (syscall(__NR_riscv_hwprobe, &pair, 1, 0, NULL, 0) == 0) {
		have_v = (pair.key >= 0) && (pair.value & BASE64_RISCV_HWPROBE_IMA_V);
	} else {
		have_v = (getauxval(AT_HWCAP) & BASE64_RISCV_HWCAP_V) != 0;
	}

	if (have_v) {
		codec->enc = base64_stream_encode_rvv;
		codec->dec = base64_stream_decode_rvv;
		return true;
	}

#else
	(void)codec;
#endif

	return false;
}

static bool
codec_choose_x86 (struct codec *codec)
{
#ifdef BASE64_X86_SIMD

	unsigned int eax, ebx = 0, ecx = 0, edx;
	unsigned int max_level;

	#ifdef _MSC_VER
	int info[4];
	__cpuidex(info, 0, 0);
	max_level = info[0];
	#else
	max_level = __get_cpuid_max(0, NULL);
	#endif

	#if HAVE_AVX512 || HAVE_AVX2 || HAVE_AVX
	// Check for AVX/AVX2/AVX512 support:
	// Checking for AVX requires 3 things:
	// 1) CPUID indicates that the OS uses XSAVE and XRSTORE instructions
	//    (allowing saving YMM registers on context switch)
	// 2) CPUID indicates support for AVX
	// 3) XGETBV indicates the AVX registers will be saved and restored on
	//    context switch
	//
	// Note that XGETBV is only available on 686 or later CPUs, so the
	// instruction needs to be conditionally run.
	if (max_level >= 1) {
		__cpuid_count(1, 0, eax, ebx, ecx, edx);
		if (ecx & bit_XSAVE_XRSTORE) {
			uint64_t xcr_mask;
			xcr_mask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
			if ((xcr_mask & _XCR_XMM_AND_YMM_STATE_ENABLED_BY_OS) == _XCR_XMM_AND_YMM_STATE_ENABLED_BY_OS) { // check multiple bits at once
				#if HAVE_AVX512
				if (max_level >= 7 && ((xcr_mask & _AVX_512_ENABLED_BY_OS) == _AVX_512_ENABLED_BY_OS)) {
					__cpuid_count(7, 0, eax, ebx, ecx, edx);
					if ((ebx & bit_AVX512vl) && (ecx & bit_AVX512vbmi)) {
						codec->enc = base64_stream_encode_avx512;
						codec->dec = base64_stream_decode_avx512;
						return true;
					}
				}
				#endif
				#if HAVE_AVX2
				if (max_level >= 7) {
					__cpuid_count(7, 0, eax, ebx, ecx, edx);
					if (ebx & bit_AVX2) {
						codec->enc = base64_stream_encode_avx2;
						codec->dec = base64_stream_decode_avx2;
						return true;
					}
				}
				#endif
				#if HAVE_AVX
				__cpuid_count(1, 0, eax, ebx, ecx, edx);
				if (ecx & bit_AVX) {
					codec->enc = base64_stream_encode_avx;
					codec->dec = base64_stream_decode_avx;
					return true;
				}
				#endif
			}
		}
	}
	#endif

	#if HAVE_SSE42
	// Check for SSE42 support:
	if (max_level >= 1) {
		__cpuid(1, eax, ebx, ecx, edx);
		if (ecx & bit_SSE42) {
			codec->enc = base64_stream_encode_sse42;
			codec->dec = base64_stream_decode_sse42;
			return true;
		}
	}
	#endif

	#if HAVE_SSE41
	// Check for SSE41 support:
	if (max_level >= 1) {
		__cpuid(1, eax, ebx, ecx, edx);
		if (ecx & bit_SSE41) {
			codec->enc = base64_stream_encode_sse41;
			codec->dec = base64_stream_decode_sse41;
			return true;
		}
	}
	#endif

	#if HAVE_SSSE3
	// Check for SSSE3 support:
	if (max_level >= 1) {
		__cpuid(1, eax, ebx, ecx, edx);
		if (ecx & bit_SSSE3) {
			codec->enc = base64_stream_encode_ssse3;
			codec->dec = base64_stream_decode_ssse3;
			return true;
		}
	}
	#endif

#else
	(void)codec;
#endif

	return false;
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
	if (codec_choose_riscv(codec)) {
		return;
	}
	codec->enc = base64_stream_encode_plain;
	codec->dec = base64_stream_decode_plain;
}
