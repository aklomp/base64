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

#ifdef BASE64_X86

int x86_get_cpu_feature_level (void)
{
	unsigned int eax, ebx, ecx, edx;
	unsigned int max_level;

#ifdef _MSC_VER
	int info[4];
	__cpuidex(info, 0, 0);
	max_level = info[0];
#else
	max_level = __get_cpuid_max(0, NULL);
#endif

	if (max_level >= 1) {

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
		__cpuid_count(1, 0, eax, ebx, ecx, edx);
		if (ecx & bit_XSAVE_XRSTORE) {
			uint64_t xcr_mask;
			xcr_mask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
			if ((xcr_mask & _XCR_XMM_AND_YMM_STATE_ENABLED_BY_OS) == _XCR_XMM_AND_YMM_STATE_ENABLED_BY_OS) { // check multiple bits at once
#if HAVE_AVX512
				if (max_level >= 7 && ((xcr_mask & _AVX_512_ENABLED_BY_OS) == _AVX_512_ENABLED_BY_OS)) {
					__cpuid_count(7, 0, eax, ebx, ecx, edx);
					if ((ebx & bit_AVX512vl) && (ecx & bit_AVX512vbmi)) {
						return X86_FEATURE_LEVEL_AVX512;
					}
				}
#endif // HAVE_AVX512

#if HAVE_AVX2
				if (max_level >= 7) {
					__cpuid_count(7, 0, eax, ebx, ecx, edx);
					if (ebx & bit_AVX2) {
						return X86_FEATURE_LEVEL_AVX2;
					}
				}
#endif // HAVE_AVX2

#if HAVE_AVX
				__cpuid_count(1, 0, eax, ebx, ecx, edx);
				if (ecx & bit_AVX) {
					return X86_FEATURE_LEVEL_AVX;
				}
#endif // HAVE_AVX
			}
		}
#endif // HAVE_AVX512 || HAVE_AVX2 || HAVE_AVX

		__cpuid(1, eax, ebx, ecx, edx);

#if HAVE_SSE42
		// Check for SSE42 support:
		if (ecx & bit_SSE42) {
			return X86_FEATURE_LEVEL_SSE42;
		}
#endif // HAVE_SSE42

#if HAVE_SSE41
		// Check for SSE41 support:
		if (ecx & bit_SSE41) {
			return X86_FEATURE_LEVEL_SSE41;
		}
#endif // HAVE_SSE41

#if HAVE_SSSE3
		// Check for SSSE3 support:
		if (ecx & bit_SSSE3) {
			return X86_FEATURE_LEVEL_SSSE3;
		}
#endif // HAVE_SSSE3
	}

	return X86_FEATURE_LEVEL_NONE;
}
#else

int x86_get_cpu_feature_level (void)
{
	return X86_FEATURE_LEVEL_NONE;
}

#endif // BASE64_X86
