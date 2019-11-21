#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../../../include/libbase64.h"
#include "../../codecs.h"

#if BASE64_WORDSIZE == 64
#  include "64/enc_loop.c"
#endif

BASE64_ENC_FUNCTION(plain)
{
	#include "enc_head.c"
#if BASE64_WORDSIZE == 32
	#include "32/enc_loop.c"
#elif BASE64_WORDSIZE == 64
	enc_loop_generic_64(&c, &srclen, &o, &outl);
#endif
	#include "enc_tail.c"
}

BASE64_DEC_FUNCTION(plain)
{
	#include "dec_head.c"
#if BASE64_WORDSIZE >= 32
	#include "32/dec_loop.c"
#endif
	#include "dec_tail.c"
}
