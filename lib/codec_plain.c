#include <stddef.h>
#include <stdint.h>

#include "../include/libbase64.h"
#include "codecs.h"

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
