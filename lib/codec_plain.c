#include <stdint.h>
#include <stddef.h>

#include "../include/libbase64.h"
#include "codecs.h"

/* Define machine endianness: */
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	#define LITTLE_ENDIAN 1
#else
	#define LITTLE_ENDIAN 0
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
