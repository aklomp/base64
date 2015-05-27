#include <stddef.h>
#ifdef __SSSE3__
#include <tmmintrin.h>
#endif

#include "../include/libbase64.h"
#include "codec_choose.h"

extern const char base64_table_enc[];
extern const unsigned char base64_table_dec[];

void
base64_stream_encode_ssse3 BASE64_ENC_PARAMS
{
#ifdef __SSSE3__
	#include "enc/head.c"
	#include "enc/ssse3.c"
	#include "enc/tail.c"
#else
	BASE64_ENC_STUB
#endif
}

int
base64_stream_decode_ssse3 BASE64_DEC_PARAMS
{
#ifdef __SSSE3__
	#include "dec/head.c"
	#include "dec/ssse3.c"
	#include "dec/tail.c"
#else
	BASE64_DEC_STUB
#endif
}
