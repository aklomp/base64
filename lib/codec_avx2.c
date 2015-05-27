#include <stddef.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif

#include "../include/libbase64.h"
#include "codecs.h"

void
base64_stream_encode_avx2 BASE64_ENC_PARAMS
{
#ifdef __AVX2__
	#include "enc/head.c"
	#include "enc/avx2.c"
	#include "enc/tail.c"
#else
	BASE64_ENC_STUB
#endif
}

int
base64_stream_decode_avx2 BASE64_DEC_PARAMS
{
#ifdef __AVX2__
	#include "dec/head.c"
	#include "dec/avx2.c"
	#include "dec/tail.c"
#else
	BASE64_DEC_STUB
#endif
}
