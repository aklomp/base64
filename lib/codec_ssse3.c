#include <stdint.h>
#include <stddef.h>
#ifdef __SSSE3__
#include <tmmintrin.h>
#endif

#include "../include/libbase64.h"
#include "codecs.h"

BASE64_ENC_FUNCTION(ssse3)
{
#ifdef __SSSE3__
	#include "enc/head.c"
	#include "enc/ssse3.c"
	#include "enc/tail.c"
#else
	BASE64_ENC_STUB
#endif
}

BASE64_DEC_FUNCTION(ssse3)
{
#ifdef __SSSE3__
	#include "dec/head.c"
	#include "dec/ssse3.c"
	#include "dec/tail.c"
#else
	BASE64_DEC_STUB
#endif
}
