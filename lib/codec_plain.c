#include <stdint.h>
#include <stddef.h>

#include "../include/libbase64.h"
#include "codecs.h"

// Define machine endianness:
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	#define LITTLE_ENDIAN 1
#else
	#define LITTLE_ENDIAN 0
#endif

// Endian conversion functions
#if LITTLE_ENDIAN
	#define cpu_to_be32(x)	__builtin_bswap32(x)
	#define cpu_to_be64(x)	__builtin_bswap64(x)
	#define be32_to_cpu(x)	__builtin_bswap32(x)
	#define be64_to_cpu(x)	__builtin_bswap64(x)
#else
	#define cpu_to_be32(x)	(x)
	#define cpu_to_be64(x)	(x)
	#define be32_to_cpu(x)	(x)
	#define be64_to_cpu(x)	(x)
#endif

BASE64_ENC_FUNCTION(plain)
{
	#include "enc/head.c"
#if __WORDSIZE == 32
	#include "enc/uint32.c"
#elif __WORDSIZE == 64
	#include "enc/uint64.c"
#endif
	#include "enc/tail.c"
}

BASE64_DEC_FUNCTION(plain)
{
	#include "dec/head.c"
#if __WORDSIZE == 32
	#include "dec/uint32.c"
#elif __WORDSIZE == 64
	#include "dec/uint64.c"
#endif
	#include "dec/tail.c"
}
